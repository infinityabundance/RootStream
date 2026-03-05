/*
 * congestion_stats.h — RTT + loss congestion statistics aggregator
 *
 * Combines rtt_estimator and loss_detector snapshots into a single
 * composite view plus event counters (congestion onset / recovery).
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_CONGESTION_STATS_H
#define ROOTSTREAM_CONGESTION_STATS_H

#include "rtt_estimator.h"
#include "loss_detector.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Composite congestion statistics */
typedef struct {
    rtt_snapshot_t rtt;              /**< RTT estimates */
    double         loss_fraction;    /**< Current window loss fraction */
    bool           congested;        /**< Current congestion state */
    uint64_t       congestion_events; /**< Times congestion was first detected */
    uint64_t       recovery_events;   /**< Times congestion cleared */
} congestion_snapshot_t;

/** Opaque congestion stats context */
typedef struct congestion_stats_s congestion_stats_t;

/**
 * congestion_stats_create — allocate context
 *
 * @param loss_threshold  Congestion threshold for the loss detector
 * @return                Non-NULL handle, or NULL on OOM
 */
congestion_stats_t *congestion_stats_create(double loss_threshold);

/**
 * congestion_stats_destroy — free context
 *
 * @param cs  Context to destroy
 */
void congestion_stats_destroy(congestion_stats_t *cs);

/**
 * congestion_stats_record_rtt — feed a new RTT sample
 *
 * @param cs      Context
 * @param rtt_us  RTT in µs
 * @return        0 on success, -1 on NULL / error
 */
int congestion_stats_record_rtt(congestion_stats_t *cs, uint64_t rtt_us);

/**
 * congestion_stats_record_packet — record a packet outcome
 *
 * @param cs       Context
 * @param outcome  RECEIVED or LOST
 * @return         LOSS_SIGNAL_* from underlying detector
 */
loss_signal_t congestion_stats_record_packet(congestion_stats_t *cs,
                                               loss_outcome_t      outcome);

/**
 * congestion_stats_snapshot — copy current statistics
 *
 * @param cs   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int congestion_stats_snapshot(const congestion_stats_t *cs,
                                congestion_snapshot_t    *out);

/**
 * congestion_stats_reset — clear all statistics
 *
 * @param cs  Context
 */
void congestion_stats_reset(congestion_stats_t *cs);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CONGESTION_STATS_H */
