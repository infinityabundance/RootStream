/*
 * rm_table.h — Retry Manager: 32-slot retry table
 *
 * Manages a bounded set of rm_entry_t instances.  The `tick(now_us)`
 * function scans for entries that are due and returns them to the
 * caller via a callback; the caller then decides whether to mark the
 * request as succeeded (remove) or failed (let it expire on the next
 * tick after max_attempts).
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_RM_TABLE_H
#define ROOTSTREAM_RM_TABLE_H

#include <stddef.h>

#include "rm_entry.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RM_MAX_SLOTS 32 /**< Maximum tracked requests */

/** Opaque retry table */
typedef struct rm_table_s rm_table_t;

/**
 * rm_table_create — allocate table
 *
 * @return Non-NULL handle, or NULL on OOM
 */
rm_table_t *rm_table_create(void);

/**
 * rm_table_destroy — free table
 */
void rm_table_destroy(rm_table_t *t);

/**
 * rm_table_add — register a new request for retry management
 *
 * @param t             Table
 * @param request_id    Unique request ID
 * @param now_us        Current wall-clock µs
 * @param base_delay_us Initial back-off interval (µs)
 * @param max_attempts  Maximum attempts (> 0)
 * @return              Pointer to new entry (owned by table), or NULL if full
 */
rm_entry_t *rm_table_add(rm_table_t *t, uint64_t request_id, uint64_t now_us,
                         uint64_t base_delay_us, uint32_t max_attempts);

/**
 * rm_table_remove — remove a request by ID
 *
 * @param t          Table
 * @param request_id Request to remove
 * @return           0 on success, -1 if not found
 */
int rm_table_remove(rm_table_t *t, uint64_t request_id);

/**
 * rm_table_get — look up a request by ID
 *
 * @return Pointer to entry (owned by table), or NULL if not found
 */
rm_entry_t *rm_table_get(rm_table_t *t, uint64_t request_id);

/**
 * rm_table_count — number of active entries
 */
int rm_table_count(const rm_table_t *t);

/**
 * rm_table_tick — fire callbacks for all due entries
 *
 * For each entry where `now_us >= next_retry_us`:
 * - Invokes `cb(entry, user)`
 * - Calls `rm_entry_advance()` on the entry; if max_attempts reached,
 *   removes the entry automatically.
 *
 * @param t      Table
 * @param now_us Current wall-clock µs
 * @param cb     Called for each due entry (may be NULL to just expire)
 * @param user   Passed through to cb
 * @return       Number of entries processed
 */
int rm_table_tick(rm_table_t *t, uint64_t now_us, void (*cb)(rm_entry_t *e, void *user),
                  void *user);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_RM_TABLE_H */
