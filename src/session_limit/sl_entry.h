/*
 * sl_entry.h — Single session entry
 *
 * Represents one active session tracked by the session limiter:
 * a unique 64-bit session ID, the remote IP string, the start
 * timestamp, and the current state.
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_SL_ENTRY_H
#define ROOTSTREAM_SL_ENTRY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SL_IP_MAX  48   /**< Max remote IP string length (IPv6) */

/** Session state */
typedef enum {
    SL_CONNECTING = 0,
    SL_ACTIVE     = 1,
    SL_CLOSING    = 2,
} sl_state_t;

/** Single session entry */
typedef struct {
    uint64_t   session_id;
    char       remote_ip[SL_IP_MAX];
    uint64_t   start_us;
    sl_state_t state;
    bool       in_use;
} sl_entry_t;

/**
 * sl_entry_init — initialise a session entry
 *
 * @param e          Entry
 * @param session_id Unique session identifier
 * @param remote_ip  Remote IP string (truncated to SL_IP_MAX-1)
 * @param start_us   Session start time (µs)
 * @return           0 on success, -1 on NULL
 */
int sl_entry_init(sl_entry_t *e,
                   uint64_t    session_id,
                   const char *remote_ip,
                   uint64_t    start_us);

/**
 * sl_state_name — human-readable state string
 */
const char *sl_state_name(sl_state_t s);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SL_ENTRY_H */
