/*
 * eb_stats.h — Event Bus statistics
 *
 * Tracks aggregate publication, dispatch, and drop counts across all
 * event types.  Per-type counters are maintained for the N most recent
 * distinct types seen.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_EB_STATS_H
#define ROOTSTREAM_EB_STATS_H

#include <stdint.h>

#include "eb_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Event bus statistics snapshot */
typedef struct {
    uint64_t published_count; /**< Total events published */
    uint64_t dispatch_count;  /**< Total subscriber invocations */
    uint64_t dropped_count;   /**< Published events with 0 subscribers */
} eb_stats_snapshot_t;

/** Opaque event bus stats context */
typedef struct eb_stats_s eb_stats_t;

/**
 * eb_stats_create — allocate context
 *
 * @return Non-NULL handle, or NULL on OOM
 */
eb_stats_t *eb_stats_create(void);

/**
 * eb_stats_destroy — free context
 */
void eb_stats_destroy(eb_stats_t *st);

/**
 * eb_stats_record_publish — record one publish operation
 *
 * @param st            Context
 * @param dispatch_n    Number of subscribers dispatched to (0 = dropped)
 * @return              0 on success, -1 on NULL
 */
int eb_stats_record_publish(eb_stats_t *st, int dispatch_n);

/**
 * eb_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int eb_stats_snapshot(const eb_stats_t *st, eb_stats_snapshot_t *out);

/**
 * eb_stats_reset — clear all statistics
 */
void eb_stats_reset(eb_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_EB_STATS_H */
