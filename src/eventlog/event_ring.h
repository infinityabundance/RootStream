/*
 * event_ring.h — Fixed-capacity overwriting circular event log
 *
 * Stores the last EVENT_RING_CAPACITY entries in a ring buffer.
 * When full, the oldest entry is silently overwritten.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_EVENT_RING_H
#define ROOTSTREAM_EVENT_RING_H

#include <stdbool.h>
#include <stddef.h>

#include "event_entry.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_RING_CAPACITY 256 /**< Maximum entries in the ring */

/** Opaque event ring */
typedef struct event_ring_s event_ring_t;

/**
 * event_ring_create — allocate ring
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
event_ring_t *event_ring_create(void);

/**
 * event_ring_destroy — free ring
 *
 * @param r  Ring to destroy
 */
void event_ring_destroy(event_ring_t *r);

/**
 * event_ring_push — append an entry (overwrites oldest when full)
 *
 * @param r  Ring
 * @param e  Entry to append
 * @return   0 on success, -1 on NULL args
 */
int event_ring_push(event_ring_t *r, const event_entry_t *e);

/**
 * event_ring_count — number of stored entries
 *
 * @param r  Ring
 * @return   Count (0 to EVENT_RING_CAPACITY)
 */
int event_ring_count(const event_ring_t *r);

/**
 * event_ring_is_empty — return true if no entries
 *
 * @param r  Ring
 * @return   true if empty
 */
bool event_ring_is_empty(const event_ring_t *r);

/**
 * event_ring_get — retrieve entry by age (0 = newest, count-1 = oldest)
 *
 * @param r    Ring
 * @param age  Age index
 * @param out  Output entry
 * @return     0 on success, -1 if out of range
 */
int event_ring_get(const event_ring_t *r, int age, event_entry_t *out);

/**
 * event_ring_clear — remove all entries
 *
 * @param r  Ring
 */
void event_ring_clear(event_ring_t *r);

/**
 * event_ring_find_level — find entries at or above @min_level
 *
 * Iterates from newest to oldest; stores up to @max_results indices
 * (into the ring, by age) in @out_ages.
 *
 * @param r            Ring
 * @param min_level    Minimum level to match
 * @param out_ages     Output array of age indices
 * @param max_results  Maximum results to return
 * @return             Number of matches found
 */
int event_ring_find_level(const event_ring_t *r, event_level_t min_level, int *out_ages,
                          int max_results);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_EVENT_RING_H */
