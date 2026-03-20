/*
 * reorder_stats.h — Packet reorder buffer statistics
 *
 * Tracks reorder depth (max observed gap between expected and earliest
 * held seq), late packet deliveries (timed-out flushes), and discards
 * (buffer-full drops).
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_REORDER_STATS_H
#define ROOTSTREAM_REORDER_STATS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Reorder statistics snapshot */
typedef struct {
    uint64_t packets_inserted;  /**< Total packets inserted */
    uint64_t packets_delivered; /**< Total packets delivered (in-order) */
    uint64_t late_flushes;      /**< Packets delivered via timeout flush */
    uint64_t discards;          /**< Insert failures (buffer full / dup) */
    int max_depth;              /**< Maximum observed reorder depth (seq gaps) */
} reorder_stats_snapshot_t;

/** Opaque reorder stats context */
typedef struct reorder_stats_s reorder_stats_t;

/**
 * reorder_stats_create — allocate stats context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
reorder_stats_t *reorder_stats_create(void);

/**
 * reorder_stats_destroy — free context
 *
 * @param st  Context to destroy
 */
void reorder_stats_destroy(reorder_stats_t *st);

/**
 * reorder_stats_record_insert — record an insert attempt
 *
 * @param st      Context
 * @param success 1 if inserted, 0 if discarded
 * @param depth   Current buffer depth (occupied slot count after insert)
 * @return        0 on success, -1 on NULL
 */
int reorder_stats_record_insert(reorder_stats_t *st, int success, int depth);

/**
 * reorder_stats_record_deliver — record a delivery
 *
 * @param st        Context
 * @param timed_out 1 if this was a timeout flush, 0 if in-order
 * @return          0 on success, -1 on NULL
 */
int reorder_stats_record_deliver(reorder_stats_t *st, int timed_out);

/**
 * reorder_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int reorder_stats_snapshot(const reorder_stats_t *st, reorder_stats_snapshot_t *out);

/**
 * reorder_stats_reset — clear all statistics
 *
 * @param st  Context
 */
void reorder_stats_reset(reorder_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_REORDER_STATS_H */
