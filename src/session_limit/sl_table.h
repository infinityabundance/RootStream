/*
 * sl_table.h — 32-slot session table
 *
 * Manages a bounded table of sl_entry_t instances with a configurable
 * maximum concurrent session count.  Attempting to add beyond the cap
 * fails and increments the rejection counter in the associated stats.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_SL_TABLE_H
#define ROOTSTREAM_SL_TABLE_H

#include "sl_entry.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SL_MAX_SLOTS   32   /**< Hard upper limit of tracked sessions */

/** Opaque session table */
typedef struct sl_table_s sl_table_t;

/**
 * sl_table_create — allocate table
 *
 * @param max_sessions  Maximum concurrent sessions (1..SL_MAX_SLOTS)
 * @return              Non-NULL handle, or NULL on OOM/invalid
 */
sl_table_t *sl_table_create(int max_sessions);

/**
 * sl_table_destroy — free table
 */
void sl_table_destroy(sl_table_t *t);

/**
 * sl_table_add — admit a new session
 *
 * @param t          Table
 * @param session_id Unique session ID
 * @param remote_ip  Remote IP string
 * @param start_us   Start timestamp (µs)
 * @return           Pointer to new entry, or NULL if cap reached / OOM
 */
sl_entry_t *sl_table_add(sl_table_t *t,
                           uint64_t    session_id,
                           const char *remote_ip,
                           uint64_t    start_us);

/**
 * sl_table_remove — remove session by ID
 *
 * @param t          Table
 * @param session_id Session to remove
 * @return           0 on success, -1 if not found
 */
int sl_table_remove(sl_table_t *t, uint64_t session_id);

/**
 * sl_table_get — look up session by ID
 *
 * @return  Pointer to entry (owned by table), or NULL
 */
sl_entry_t *sl_table_get(sl_table_t *t, uint64_t session_id);

/**
 * sl_table_count — current number of active sessions
 */
int sl_table_count(const sl_table_t *t);

/**
 * sl_table_foreach — iterate active entries
 */
void sl_table_foreach(sl_table_t *t,
                       void (*cb)(sl_entry_t *e, void *user),
                       void *user);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SL_TABLE_H */
