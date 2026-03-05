/*
 * ts_stats.h — Timestamp synchronizer statistics
 *
 * Tracks the number of anchor updates, peak drift observed, and total
 * correction applied since the last reset.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_TS_STATS_H
#define ROOTSTREAM_TS_STATS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Timestamp statistics snapshot */
typedef struct {
    uint64_t sample_count;       /**< Drift measurements taken */
    int64_t  max_drift_us;       /**< Maximum |error| observed (µs) */
    int64_t  total_correction_us;/**< Sum of all |error| values (µs) */
} ts_stats_snapshot_t;

/** Opaque timestamp stats context */
typedef struct ts_stats_s ts_stats_t;

/**
 * ts_stats_create — allocate context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
ts_stats_t *ts_stats_create(void);

/**
 * ts_stats_destroy — free context
 */
void ts_stats_destroy(ts_stats_t *st);

/**
 * ts_stats_record — record one drift measurement
 *
 * @param st       Context
 * @param error_us Signed error (observed_us − expected_us)
 * @return         0 on success, -1 on NULL
 */
int ts_stats_record(ts_stats_t *st, int64_t error_us);

/**
 * ts_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int ts_stats_snapshot(const ts_stats_t *st, ts_stats_snapshot_t *out);

/**
 * ts_stats_reset — clear all statistics
 */
void ts_stats_reset(ts_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_TS_STATS_H */
