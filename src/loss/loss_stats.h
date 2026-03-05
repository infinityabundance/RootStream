/*
 * loss_stats.h — Packet loss detailed statistics
 *
 * Tracks totals, burst sizes, and a per-second loss rate for
 * reporting and congestion control feedback.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_LOSS_STATS_H
#define ROOTSTREAM_LOSS_STATS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Loss statistics snapshot */
typedef struct {
    uint64_t total_sent;     /**< Total sequence numbers observed */
    uint64_t total_lost;     /**< Total packets counted as lost */
    uint32_t burst_count;    /**< Number of loss bursts (consecutive losses) */
    uint32_t max_burst;      /**< Longest single burst (packets) */
    double   loss_pct;       /**< total_lost/total_sent × 100 */
} loss_stats_snapshot_t;

/** Opaque loss statistics context */
typedef struct loss_stats_s loss_stats_t;

/**
 * loss_stats_create — allocate context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
loss_stats_t *loss_stats_create(void);

/**
 * loss_stats_destroy — free context
 */
void loss_stats_destroy(loss_stats_t *st);

/**
 * loss_stats_record — record one packet outcome
 *
 * @param st      Context
 * @param lost    1 if this packet was lost, 0 if received
 * @return        0 on success, -1 on NULL
 */
int loss_stats_record(loss_stats_t *st, int lost);

/**
 * loss_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int loss_stats_snapshot(const loss_stats_t *st, loss_stats_snapshot_t *out);

/**
 * loss_stats_reset — clear all statistics
 */
void loss_stats_reset(loss_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_LOSS_STATS_H */
