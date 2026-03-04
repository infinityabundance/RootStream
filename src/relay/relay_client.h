/*
 * relay_client.h — Client-side relay connector
 *
 * Manages the client's connection to a relay server: connects, sends
 * HELLO, waits for HELLO_ACK, then enters data-relay mode.
 *
 * The connector is designed to be non-blocking compatible; all I/O
 * is done via callbacks supplied at creation time.  For testing the
 * callbacks write to a buffer instead of real sockets.
 */

#ifndef ROOTSTREAM_RELAY_CLIENT_H
#define ROOTSTREAM_RELAY_CLIENT_H

#include "relay_protocol.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Relay client connection state */
typedef enum {
    RELAY_CLIENT_DISCONNECTED = 0,
    RELAY_CLIENT_CONNECTING   = 1,
    RELAY_CLIENT_HELLO_SENT   = 2,
    RELAY_CLIENT_READY        = 3,  /**< Fully paired and relaying */
    RELAY_CLIENT_ERROR        = 4,
} relay_client_state_t;

/** I/O callbacks provided by the host application */
typedef struct {
    /**
     * send_fn — write @len bytes of @data to the relay server
     * Returns number of bytes sent, or -1 on error.
     */
    int (*send_fn)(const uint8_t *data, size_t len, void *user_data);

    void *user_data;
} relay_io_t;

/** Opaque relay client handle */
typedef struct relay_client_s relay_client_t;

/**
 * relay_client_create — allocate relay client
 *
 * @param io       I/O callbacks (send_fn must be non-NULL)
 * @param token    32-byte auth token for this session
 * @param is_host  true = host role, false = viewer
 * @return         Non-NULL handle, or NULL on failure
 */
relay_client_t *relay_client_create(const relay_io_t *io,
                                     const uint8_t    *token,
                                     bool              is_host);

/**
 * relay_client_destroy — free relay client
 *
 * @param client  Client to destroy
 */
void relay_client_destroy(relay_client_t *client);

/**
 * relay_client_connect — send HELLO to the relay server
 *
 * Transitions state from DISCONNECTED → HELLO_SENT.
 *
 * @param client  Relay client
 * @return        0 on success, -1 on I/O error or wrong state
 */
int relay_client_connect(relay_client_t *client);

/**
 * relay_client_receive — process @len bytes received from the relay server
 *
 * Parses the relay header and handles:
 *   HELLO_ACK  → transitions to READY
 *   DATA       → calls @data_cb with the payload
 *   PING       → sends PONG automatically
 *   DISCONNECT → transitions to DISCONNECTED
 *
 * @param client      Relay client
 * @param buf         Received bytes
 * @param len         Length of @buf
 * @param data_cb     Called when a DATA message is received
 * @param data_ud     user_data passed to data_cb
 * @return            0 on success, -1 on parse error
 */
int relay_client_receive(relay_client_t *client,
                          const uint8_t  *buf,
                          size_t          len,
                          void (*data_cb)(const uint8_t *, size_t, void *),
                          void           *data_ud);

/**
 * relay_client_send_data — send a data payload through the relay server
 *
 * Wraps @payload in a RELAY_MSG_DATA message and sends it.
 *
 * @param client       Relay client
 * @param payload      Data to relay
 * @param payload_len  Size in bytes
 * @return             0 on success, -1 on error or not READY
 */
int relay_client_send_data(relay_client_t *client,
                            const uint8_t  *payload,
                            size_t          payload_len);

/**
 * relay_client_get_state — return current connection state
 *
 * @param client  Relay client
 * @return        Current state
 */
relay_client_state_t relay_client_get_state(const relay_client_t *client);

/**
 * relay_client_get_session_id — return server-assigned session ID
 *
 * Valid only in READY state; returns 0 otherwise.
 *
 * @param client  Relay client
 * @return        Session ID
 */
relay_session_id_t relay_client_get_session_id(const relay_client_t *client);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_RELAY_CLIENT_H */
