/*
 * pq_stats.h — Priority Queue statistics
 *
 * Tracks push/pop counts, the peak heap size, and the number of push
 * attempts that were rejected because the heap was full.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_PQ_STATS_H
#define ROOTSTREAM_PQ_STATS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Priority queue statistics snapshot */
typedef struct {
    uint64_t push_count;     /**< Total successful pushes */
    uint64_t pop_count;      /**< Total successful pops */
    int peak_size;           /**< Maximum simultaneous heap occupancy */
    uint64_t overflow_count; /**< Push rejections (heap full) */
} pq_stats_snapshot_t;

/** Opaque priority queue stats context */
typedef struct pq_stats_s pq_stats_t;

/**
 * pq_stats_create — allocate context
 *
 * @return Non-NULL handle, or NULL on OOM
 */
pq_stats_t *pq_stats_create(void);

/**
 * pq_stats_destroy — free context
 */
void pq_stats_destroy(pq_stats_t *st);

/**
 * pq_stats_record_push — record a successful push
 *
 * @param st        Context
 * @param cur_size  Current heap occupancy after push
 * @return          0 on success, -1 on NULL
 */
int pq_stats_record_push(pq_stats_t *st, int cur_size);

/**
 * pq_stats_record_pop — record a successful pop
 *
 * @param st  Context
 * @return    0 on success, -1 on NULL
 */
int pq_stats_record_pop(pq_stats_t *st);

/**
 * pq_stats_record_overflow — record a failed push (heap full)
 *
 * @param st  Context
 * @return    0 on success, -1 on NULL
 */
int pq_stats_record_overflow(pq_stats_t *st);

/**
 * pq_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int pq_stats_snapshot(const pq_stats_t *st, pq_stats_snapshot_t *out);

/**
 * pq_stats_reset — clear all statistics
 */
void pq_stats_reset(pq_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PQ_STATS_H */
