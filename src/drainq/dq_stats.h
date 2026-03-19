/*
 * dq_stats.h — Drain Queue statistics
 *
 * Tracks enqueue/drain/drop counts and peak queue depth.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_DQ_STATS_H
#define ROOTSTREAM_DQ_STATS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Drain queue statistics snapshot */
typedef struct {
    uint64_t enqueued; /**< Total successful enqueue calls */
    uint64_t drained;  /**< Total entries removed by dequeue/drain_all */
    uint64_t dropped;  /**< Enqueue rejections (queue full) */
    int peak;          /**< Maximum simultaneous queue depth */
} dq_stats_snapshot_t;

/** Opaque drain queue stats context */
typedef struct dq_stats_s dq_stats_t;

dq_stats_t *dq_stats_create(void);
void dq_stats_destroy(dq_stats_t *st);

int dq_stats_record_enqueue(dq_stats_t *st, int cur_depth);
int dq_stats_record_drain(dq_stats_t *st);
int dq_stats_record_drop(dq_stats_t *st);
int dq_stats_snapshot(const dq_stats_t *st, dq_stats_snapshot_t *out);
void dq_stats_reset(dq_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_DQ_STATS_H */
