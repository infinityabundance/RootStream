/*
 * bp_stats.h — Buffer pool statistics
 *
 * Tracks allocation / free counts, the peak in-use count, and the
 * number of allocation failures (all blocks busy) for capacity
 * planning.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_BP_STATS_H
#define ROOTSTREAM_BP_STATS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Buffer pool stats snapshot */
typedef struct {
    uint64_t alloc_count;   /**< Total successful acquires */
    uint64_t free_count;    /**< Total successful releases */
    int      peak_in_use;   /**< Highest simultaneous in-use count */
    uint64_t fail_count;    /**< Acquire failures (pool exhausted) */
} bp_stats_snapshot_t;

/** Opaque buffer pool stats context */
typedef struct bp_stats_s bp_stats_t;

/**
 * bp_stats_create — allocate context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
bp_stats_t *bp_stats_create(void);

/**
 * bp_stats_destroy — free context
 */
void bp_stats_destroy(bp_stats_t *st);

/**
 * bp_stats_record_alloc — record a successful acquire
 *
 * @param st        Context
 * @param in_use    Current in-use count (after acquire)
 * @return          0 on success, -1 on NULL
 */
int bp_stats_record_alloc(bp_stats_t *st, int in_use);

/**
 * bp_stats_record_free — record a successful release
 *
 * @param st  Context
 * @return    0 on success, -1 on NULL
 */
int bp_stats_record_free(bp_stats_t *st);

/**
 * bp_stats_record_fail — record a failed acquire
 *
 * @param st  Context
 * @return    0 on success, -1 on NULL
 */
int bp_stats_record_fail(bp_stats_t *st);

/**
 * bp_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int bp_stats_snapshot(const bp_stats_t *st, bp_stats_snapshot_t *out);

/**
 * bp_stats_reset — clear all statistics
 */
void bp_stats_reset(bp_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_BP_STATS_H */
