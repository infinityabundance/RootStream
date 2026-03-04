/*
 * abr_stats.h — Per-session ABR statistics
 *
 * Tracks quality switches, cumulative time at each level, stall events
 * (when no bitrate estimate is available), and average quality score.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_ABR_STATS_H
#define ROOTSTREAM_ABR_STATS_H

#include "abr_ladder.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Snapshot of ABR session statistics */
typedef struct {
    uint64_t total_ticks;           /**< Total tick count */
    uint64_t upgrade_count;         /**< Number of quality upgrades */
    uint64_t downgrade_count;       /**< Number of quality downgrades */
    uint64_t stall_ticks;           /**< Ticks spent below level 1 */
    uint64_t ticks_per_level[ABR_LADDER_MAX_LEVELS]; /**< Ticks at each level */
    double   avg_level;             /**< Time-weighted average level index */
} abr_stats_snapshot_t;

/** Opaque ABR stats context */
typedef struct abr_stats_s abr_stats_t;

/**
 * abr_stats_create — allocate stats context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
abr_stats_t *abr_stats_create(void);

/**
 * abr_stats_destroy — free context
 *
 * @param st  Context to destroy
 */
void abr_stats_destroy(abr_stats_t *st);

/**
 * abr_stats_record — record one ABR decision tick
 *
 * @param st          Stats context
 * @param level_idx   Current level index after decision
 * @param prev_idx    Level index before decision
 * @param is_stall    True if BW estimator wasn't ready
 * @return            0 on success, -1 on NULL args
 */
int abr_stats_record(abr_stats_t *st,
                      int          level_idx,
                      int          prev_idx,
                      int          is_stall);

/**
 * abr_stats_snapshot — copy current statistics
 *
 * @param st   Stats context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL args
 */
int abr_stats_snapshot(const abr_stats_t *st, abr_stats_snapshot_t *out);

/**
 * abr_stats_reset — clear all accumulators
 *
 * @param st  Stats context
 */
void abr_stats_reset(abr_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_ABR_STATS_H */
