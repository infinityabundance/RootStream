/*
 * hr_stats.h — Plugin hot-reload statistics
 *
 * Tracks reload counts, failure counts, and the timestamp of the most
 * recent successful reload.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_HR_STATS_H
#define ROOTSTREAM_HR_STATS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Hot-reload statistics snapshot */
typedef struct {
    uint64_t reload_count;     /**< Total successful reloads */
    uint64_t fail_count;       /**< Total failed reload attempts */
    uint64_t last_reload_us;   /**< Timestamp of last success (µs) */
    int      loaded_plugins;   /**< Currently loaded plugin count */
} hr_stats_snapshot_t;

/** Opaque hot-reload stats context */
typedef struct hr_stats_s hr_stats_t;

/**
 * hr_stats_create — allocate stats context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
hr_stats_t *hr_stats_create(void);

/**
 * hr_stats_destroy — free context
 */
void hr_stats_destroy(hr_stats_t *st);

/**
 * hr_stats_record_reload — record one reload attempt
 *
 * @param st       Context
 * @param success  1 if reload succeeded, 0 if failed
 * @param now_us   Current timestamp in µs
 * @return         0 on success, -1 on NULL
 */
int hr_stats_record_reload(hr_stats_t *st, int success, uint64_t now_us);

/**
 * hr_stats_set_loaded — update currently-loaded plugin count
 *
 * @param st    Context
 * @param count Current count
 * @return      0 on success, -1 on NULL
 */
int hr_stats_set_loaded(hr_stats_t *st, int count);

/**
 * hr_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int hr_stats_snapshot(const hr_stats_t *st, hr_stats_snapshot_t *out);

/**
 * hr_stats_reset — clear all statistics
 */
void hr_stats_reset(hr_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_HR_STATS_H */
