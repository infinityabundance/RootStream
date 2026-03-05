/*
 * session_table.h — Per-client session state table
 *
 * Maintains a fixed-size table of active streaming sessions.  Each entry
 * tracks connection state, per-client adaptive bitrate (ABR) targets,
 * and a socket file descriptor for the transport layer.
 *
 * Thread-safety: all public functions acquire the table's internal lock.
 */

#ifndef ROOTSTREAM_SESSION_TABLE_H
#define ROOTSTREAM_SESSION_TABLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum concurrent sessions supported */
#define SESSION_TABLE_MAX 32

/** Unique session identifier */
typedef uint32_t session_id_t;

/** Session connection state */
typedef enum {
    SESSION_STATE_IDLE        = 0,
    SESSION_STATE_CONNECTING  = 1,
    SESSION_STATE_ACTIVE      = 2,
    SESSION_STATE_DRAINING    = 3,
    SESSION_STATE_CLOSED      = 4,
} session_state_t;

/** Per-client session record */
typedef struct {
    session_id_t   id;
    session_state_t state;
    int            socket_fd;        /**< Transport socket (-1 = none) */
    char           peer_addr[48];    /**< Textual address of peer */
    uint32_t       bitrate_kbps;     /**< Current ABR target */
    uint32_t       max_bitrate_kbps; /**< Negotiated ceiling */
    uint64_t       bytes_sent;       /**< Monotonic bytes counter */
    uint64_t       frames_sent;
    uint64_t       connected_at_us;  /**< Connection timestamp (monotonic µs) */
    uint32_t       rtt_ms;           /**< Last measured RTT */
    float          loss_rate;        /**< Packet loss 0.0–1.0 */
} session_entry_t;

/** Opaque session table handle */
typedef struct session_table_s session_table_t;

/**
 * session_table_create — allocate and initialise an empty table
 *
 * @return Non-NULL handle, or NULL on OOM
 */
session_table_t *session_table_create(void);

/**
 * session_table_destroy — free all resources
 *
 * Does not close any file descriptors; callers must drain sessions first.
 *
 * @param table  Table to destroy
 */
void session_table_destroy(session_table_t *table);

/**
 * session_table_add — register a new session and assign an ID
 *
 * @param table      Session table
 * @param socket_fd  Connected transport socket
 * @param peer_addr  Textual peer address (e.g. "192.168.1.5:47920")
 * @param out_id     Receives the assigned session ID on success
 * @return           0 on success, -1 if table is full or args invalid
 */
int session_table_add(session_table_t *table,
                      int              socket_fd,
                      const char      *peer_addr,
                      session_id_t    *out_id);

/**
 * session_table_remove — mark a session as closed and release the slot
 *
 * @param table  Session table
 * @param id     Session ID to remove
 * @return       0 on success, -1 if ID not found
 */
int session_table_remove(session_table_t *table, session_id_t id);

/**
 * session_table_get — copy a session record by ID
 *
 * @param table  Session table
 * @param id     Session ID to look up
 * @param out    Receives a snapshot of the session entry
 * @return       0 on success, -1 if not found
 */
int session_table_get(const session_table_t *table,
                      session_id_t           id,
                      session_entry_t       *out);

/**
 * session_table_update_bitrate — update ABR bitrate for a session
 *
 * @param table         Session table
 * @param id            Session ID
 * @param bitrate_kbps  New bitrate target
 * @return              0 on success, -1 if not found
 */
int session_table_update_bitrate(session_table_t *table,
                                 session_id_t     id,
                                 uint32_t         bitrate_kbps);

/**
 * session_table_update_stats — update network stats for a session
 *
 * @param table      Session table
 * @param id         Session ID
 * @param rtt_ms     Latest RTT measurement
 * @param loss_rate  Packet loss fraction 0.0–1.0
 * @return           0 on success, -1 if not found
 */
int session_table_update_stats(session_table_t *table,
                               session_id_t     id,
                               uint32_t         rtt_ms,
                               float            loss_rate);

/**
 * session_table_count — return number of active sessions
 *
 * @param table  Session table
 * @return       Active session count (STATE_ACTIVE only)
 */
size_t session_table_count(const session_table_t *table);

/**
 * session_table_foreach — iterate active sessions
 *
 * @param table     Session table
 * @param callback  Called for each ACTIVE session entry
 * @param user_data Passed through to @callback
 */
void session_table_foreach(const session_table_t *table,
                           void (*callback)(const session_entry_t *entry,
                                            void *user_data),
                           void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SESSION_TABLE_H */
