/*
 * scheduler.h — Stream scheduler engine
 *
 * Manages a sorted list of schedule_entry_t items and fires a callback
 * when an entry's start time arrives.  The scheduler does NOT spawn
 * threads; callers drive it by calling `scheduler_tick()` periodically
 * (e.g. from a timer loop or dedicated scheduler thread).
 *
 * Thread-safety: all public functions are protected by an internal mutex.
 *
 * Capacity: SCHEDULER_MAX_ENTRIES simultaneous entries.
 */

#ifndef ROOTSTREAM_SCHEDULER_H
#define ROOTSTREAM_SCHEDULER_H

#include "schedule_entry.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCHEDULER_MAX_ENTRIES  256

/** Fired when a scheduled entry's start time is reached */
typedef void (*scheduler_fire_fn)(const schedule_entry_t *entry,
                                   void                   *user_data);

/** Opaque scheduler handle */
typedef struct scheduler_s scheduler_t;

/**
 * scheduler_create — allocate scheduler
 *
 * @param fire_fn   Callback called when an entry fires (may be NULL)
 * @param user_data Passed to fire_fn
 * @return          Non-NULL handle, or NULL on OOM
 */
scheduler_t *scheduler_create(scheduler_fire_fn fire_fn, void *user_data);

/**
 * scheduler_destroy — free scheduler
 *
 * @param sched  Scheduler to destroy
 */
void scheduler_destroy(scheduler_t *sched);

/**
 * scheduler_add — add an entry to the schedule
 *
 * Assigns a unique ID to @entry->id.  Entry is copied.
 *
 * @param sched  Scheduler
 * @param entry  Entry to add (id field is overwritten)
 * @return       Assigned entry ID (>= 1), or 0 on failure
 */
uint64_t scheduler_add(scheduler_t *sched, schedule_entry_t *entry);

/**
 * scheduler_remove — remove an entry by ID
 *
 * @param sched  Scheduler
 * @param id     Entry ID to remove
 * @return       0 on success, -1 if not found
 */
int scheduler_remove(scheduler_t *sched, uint64_t id);

/**
 * scheduler_tick — advance scheduler to @now_us
 *
 * Fires all enabled entries whose start_us <= @now_us and have not yet
 * fired.  After firing, one-shot entries are marked fired; repeat
 * entries have their start_us advanced by 24 h.
 *
 * @param sched   Scheduler
 * @param now_us  Current wall-clock time in µs since Unix epoch
 * @return        Number of entries that fired
 */
int scheduler_tick(scheduler_t *sched, uint64_t now_us);

/**
 * scheduler_count — number of active (non-fired) entries
 *
 * @param sched  Scheduler
 * @return       Count
 */
size_t scheduler_count(const scheduler_t *sched);

/**
 * scheduler_get — copy entry by ID
 *
 * @param sched  Scheduler
 * @param id     Entry ID
 * @param out    Output entry
 * @return       0 on success, -1 if not found
 */
int scheduler_get(scheduler_t *sched, uint64_t id, schedule_entry_t *out);

/**
 * scheduler_clear — remove all entries
 *
 * @param sched  Scheduler
 */
void scheduler_clear(scheduler_t *sched);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SCHEDULER_H */
