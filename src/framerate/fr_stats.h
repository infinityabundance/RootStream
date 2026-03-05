/*
 * fr_stats.h — Frame rate controller statistics
 *
 * Accumulates per-frame observations: frame count, drop count, and
 * interval min/max for debugging and reporting.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_FR_STATS_H
#define ROOTSTREAM_FR_STATS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Frame rate statistics snapshot */
typedef struct {
    uint64_t frame_count;       /**< Frames produced */
    uint64_t drop_count;        /**< Frames dropped (limiter returned 0) */
    double   avg_interval_us;   /**< Average inter-frame interval (µs) */
    uint64_t min_interval_us;   /**< Minimum interval (µs) */
    uint64_t max_interval_us;   /**< Maximum interval (µs) */
} fr_stats_snapshot_t;

/** Opaque frame rate stats context */
typedef struct fr_stats_s fr_stats_t;

/**
 * fr_stats_create — allocate stats context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
fr_stats_t *fr_stats_create(void);

/**
 * fr_stats_destroy — free context
 */
void fr_stats_destroy(fr_stats_t *st);

/**
 * fr_stats_record_frame — record a produced frame with interval
 *
 * @param st          Context
 * @param interval_us Inter-frame interval in µs
 * @return            0 on success, -1 on NULL
 */
int fr_stats_record_frame(fr_stats_t *st, uint64_t interval_us);

/**
 * fr_stats_record_drop — increment drop counter
 *
 * @param st  Context
 * @return    0 on success, -1 on NULL
 */
int fr_stats_record_drop(fr_stats_t *st);

/**
 * fr_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int fr_stats_snapshot(const fr_stats_t *st, fr_stats_snapshot_t *out);

/**
 * fr_stats_reset — clear all statistics
 */
void fr_stats_reset(fr_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_FR_STATS_H */
