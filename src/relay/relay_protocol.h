/*
 * relay_protocol.h — RootStream relay wire protocol
 *
 * Defines the binary framing used between relay clients and a relay
 * server when a direct peer-to-peer connection is not possible.
 *
 * Packet layout (all integers big-endian / network byte order)
 * ─────────────────────────────────────────────────────────────
 *  Offset  Size  Field
 *  0       2     Magic   0x5253 ('RS')
 *  2       1     Version (currently 1)
 *  3       1     Message type  (relay_msg_type_t)
 *  4       4     Session ID  (relay_session_id_t)
 *  8       2     Payload length (bytes)
 *  10      N     Payload
 *
 * All relay messages MUST be preceded by a 10-byte header.
 */

#ifndef ROOTSTREAM_RELAY_PROTOCOL_H
#define ROOTSTREAM_RELAY_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RELAY_MAGIC        0x5253U   /* 'RS' */
#define RELAY_VERSION      1
#define RELAY_HDR_SIZE     10        /* bytes */
#define RELAY_MAX_PAYLOAD  65535     /* bytes */
#define RELAY_TOKEN_LEN    32        /* bytes — HMAC-SHA256 output */

/** Relay message types */
typedef enum {
    RELAY_MSG_HELLO        = 0x01,  /**< Client → server: announce intent    */
    RELAY_MSG_HELLO_ACK    = 0x02,  /**< Server → client: session assigned   */
    RELAY_MSG_CONNECT      = 0x03,  /**< Client → server: connect to peer    */
    RELAY_MSG_CONNECT_ACK  = 0x04,  /**< Server → client: peer found / ready */
    RELAY_MSG_DATA         = 0x05,  /**< Bi-directional: relayed data frame  */
    RELAY_MSG_PING         = 0x06,  /**< Keepalive ping                      */
    RELAY_MSG_PONG         = 0x07,  /**< Keepalive pong                      */
    RELAY_MSG_DISCONNECT   = 0x08,  /**< Graceful teardown                   */
    RELAY_MSG_ERROR        = 0x09,  /**< Server → client: error notification */
} relay_msg_type_t;

/** Relay session identifier */
typedef uint32_t relay_session_id_t;

/** Decoded relay message header */
typedef struct {
    relay_msg_type_t   type;
    relay_session_id_t session_id;
    uint16_t           payload_len;
} relay_header_t;

/**
 * relay_encode_header — write a 10-byte relay header into @buf
 *
 * @param hdr     Header to serialise
 * @param buf     Output buffer (must be >= RELAY_HDR_SIZE bytes)
 * @return        RELAY_HDR_SIZE on success, -1 on invalid args
 */
int relay_encode_header(const relay_header_t *hdr, uint8_t *buf);

/**
 * relay_decode_header — parse a 10-byte relay header from @buf
 *
 * @param buf     Input buffer (must be >= RELAY_HDR_SIZE bytes)
 * @param hdr     Output decoded header
 * @return        0 on success, -1 if magic/version wrong
 */
int relay_decode_header(const uint8_t *buf, relay_header_t *hdr);

/**
 * relay_build_hello — build a HELLO message payload
 *
 * The HELLO payload is: [token:32][role:1][reserved:3]
 * role: 0 = host, 1 = viewer
 *
 * @param token    32-byte auth token
 * @param is_host  true for host role, false for viewer
 * @param buf      Output buffer (must be >= 36 bytes)
 * @return         Payload length (36), or -1 on error
 */
int relay_build_hello(const uint8_t *token, bool is_host, uint8_t *buf);

/**
 * relay_parse_hello — parse a HELLO payload
 *
 * @param payload      Pointer to payload bytes
 * @param payload_len  Number of bytes
 * @param out_token    Receives 32-byte token (caller provides 32-byte buffer)
 * @param out_is_host  Receives role flag
 * @return             0 on success, -1 on malformed payload
 */
int relay_parse_hello(const uint8_t *payload, uint16_t payload_len,
                      uint8_t *out_token, bool *out_is_host);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_RELAY_PROTOCOL_H */
