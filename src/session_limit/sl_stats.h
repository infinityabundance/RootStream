/*
 * sl_stats.h — Session limiter statistics
 *
 * Tracks admission / rejection / peak / eviction counts for capacity
 * planning and monitoring dashboards.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_SL_STATS_H
#define ROOTSTREAM_SL_STATS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Session stats snapshot */
typedef struct {
    uint32_t total_admitted; /**< Sessions successfully admitted */
    uint32_t total_rejected; /**< Sessions rejected (cap exceeded) */
    uint32_t peak_count;     /**< Highest simultaneous session count */
    uint32_t eviction_count; /**< Forcibly evicted sessions */
} sl_stats_snapshot_t;

/** Opaque session stats context */
typedef struct sl_stats_s sl_stats_t;

/**
 * sl_stats_create — allocate context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
sl_stats_t *sl_stats_create(void);

/**
 * sl_stats_destroy — free context
 */
void sl_stats_destroy(sl_stats_t *st);

/**
 * sl_stats_record_admit — increment admitted count + update peak
 *
 * @param st           Context
 * @param current_count Current number of active sessions (after admit)
 * @return              0 on success, -1 on NULL
 */
int sl_stats_record_admit(sl_stats_t *st, int current_count);

/**
 * sl_stats_record_reject — increment rejected count
 *
 * @param st  Context
 * @return    0 on success, -1 on NULL
 */
int sl_stats_record_reject(sl_stats_t *st);

/**
 * sl_stats_record_eviction — increment eviction count
 *
 * @param st  Context
 * @return    0 on success, -1 on NULL
 */
int sl_stats_record_eviction(sl_stats_t *st);

/**
 * sl_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int sl_stats_snapshot(const sl_stats_t *st, sl_stats_snapshot_t *out);

/**
 * sl_stats_reset — clear all statistics
 */
void sl_stats_reset(sl_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SL_STATS_H */
