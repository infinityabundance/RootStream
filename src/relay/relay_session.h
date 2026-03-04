/*
 * relay_session.h — Relay server session manager
 *
 * Tracks active relay sessions on the server side.  A session is
 * created when a host sends HELLO; the session enters PAIRED state
 * when a viewer connects with the same token.
 *
 * Each session holds a pair of file descriptors: one for the host
 * and one for the viewer.  The server relay loop reads from one fd
 * and writes to the other (and vice-versa).
 *
 * Thread-safety: all public functions are protected by an internal
 * mutex and safe to call from multiple threads.
 */

#ifndef ROOTSTREAM_RELAY_SESSION_H
#define ROOTSTREAM_RELAY_SESSION_H

#include "relay_protocol.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum simultaneous relay sessions */
#define RELAY_SESSION_MAX 128

/** Session lifecycle state */
typedef enum {
    RELAY_STATE_IDLE      = 0,  /**< Slot free */
    RELAY_STATE_WAITING   = 1,  /**< Host connected, waiting for viewer */
    RELAY_STATE_PAIRED    = 2,  /**< Host + viewer connected: relaying */
    RELAY_STATE_CLOSING   = 3,  /**< Teardown in progress */
} relay_state_t;

/** Relay session entry */
typedef struct {
    relay_session_id_t id;
    relay_state_t      state;
    uint8_t            token[RELAY_TOKEN_LEN];  /**< Auth token */
    int                host_fd;     /**< Host socket fd  (-1 if absent) */
    int                viewer_fd;   /**< Viewer socket fd (-1 if absent) */
    uint64_t           created_us;  /**< Monotonic creation timestamp */
    uint64_t           bytes_relayed;
} relay_session_entry_t;

/** Opaque relay session manager */
typedef struct relay_session_manager_s relay_session_manager_t;

/**
 * relay_session_manager_create — allocate session manager
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
relay_session_manager_t *relay_session_manager_create(void);

/**
 * relay_session_manager_destroy — free all resources
 *
 * Does not close any fds; callers must drain sessions first.
 *
 * @param mgr  Manager to destroy
 */
void relay_session_manager_destroy(relay_session_manager_t *mgr);

/**
 * relay_session_open — create a new session for an incoming host
 *
 * @param mgr      Manager
 * @param token    32-byte auth token
 * @param host_fd  Connected host socket
 * @param out_id   Receives assigned session ID
 * @return         0 on success, -1 on failure (table full / bad args)
 */
int relay_session_open(relay_session_manager_t *mgr,
                       const uint8_t           *token,
                       int                      host_fd,
                       relay_session_id_t       *out_id);

/**
 * relay_session_pair — pair a viewer to an existing session
 *
 * Finds the WAITING session whose token matches @token and stores
 * @viewer_fd, transitioning to PAIRED state.
 *
 * @param mgr       Manager
 * @param token     32-byte auth token to match
 * @param viewer_fd Connected viewer socket
 * @param out_id    Receives the matched session ID
 * @return          0 on success, -1 if no matching WAITING session
 */
int relay_session_pair(relay_session_manager_t *mgr,
                       const uint8_t           *token,
                       int                      viewer_fd,
                       relay_session_id_t       *out_id);

/**
 * relay_session_close — mark a session as CLOSING and remove it
 *
 * @param mgr  Manager
 * @param id   Session ID
 * @return     0 on success, -1 if not found
 */
int relay_session_close(relay_session_manager_t *mgr,
                        relay_session_id_t       id);

/**
 * relay_session_get — copy a session entry by ID
 *
 * @param mgr  Manager
 * @param id   Session ID
 * @param out  Receives the session snapshot
 * @return     0 on success, -1 if not found
 */
int relay_session_get(relay_session_manager_t *mgr,
                      relay_session_id_t       id,
                      relay_session_entry_t   *out);

/**
 * relay_session_count — number of non-IDLE sessions
 *
 * @param mgr  Manager
 * @return     Count
 */
size_t relay_session_count(relay_session_manager_t *mgr);

/**
 * relay_session_add_bytes — increment bytes-relayed counter
 *
 * @param mgr    Manager
 * @param id     Session ID
 * @param bytes  Bytes to add
 * @return       0 on success, -1 if not found
 */
int relay_session_add_bytes(relay_session_manager_t *mgr,
                            relay_session_id_t       id,
                            uint64_t                 bytes);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_RELAY_SESSION_H */
