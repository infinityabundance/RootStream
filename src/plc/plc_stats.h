/*
 * plc_stats.h — Packet Loss Concealment statistics
 *
 * Tracks the number of received, lost, and concealed audio frames plus
 * a sliding-window loss rate estimate.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_PLC_STATS_H
#define ROOTSTREAM_PLC_STATS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Window size for loss-rate calculation (in frames) */
#define PLC_STATS_WINDOW 64

/** PLC statistics snapshot */
typedef struct {
    uint64_t frames_received;    /**< Total frames received (good) */
    uint64_t frames_lost;        /**< Total frames declared lost */
    uint64_t concealment_events; /**< Total concealment bursts started */
    double loss_rate;            /**< Sliding-window loss rate [0.0, 1.0] */
    int max_consecutive_loss;    /**< Longest burst of consecutive losses */
} plc_stats_snapshot_t;

/** Opaque PLC stats context */
typedef struct plc_stats_s plc_stats_t;

/**
 * plc_stats_create — allocate stats context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
plc_stats_t *plc_stats_create(void);

/**
 * plc_stats_destroy — free context
 *
 * @param st  Context to destroy
 */
void plc_stats_destroy(plc_stats_t *st);

/**
 * plc_stats_record_received — record a successfully received frame
 *
 * @param st  Stats context
 * @return    0 on success, -1 on NULL
 */
int plc_stats_record_received(plc_stats_t *st);

/**
 * plc_stats_record_lost — record a lost frame
 *
 * @param st             Stats context
 * @param is_new_burst   1 if this is the first loss in a new burst
 * @return               0 on success, -1 on NULL
 */
int plc_stats_record_lost(plc_stats_t *st, int is_new_burst);

/**
 * plc_stats_snapshot — copy current statistics
 *
 * @param st   Stats context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int plc_stats_snapshot(const plc_stats_t *st, plc_stats_snapshot_t *out);

/**
 * plc_stats_reset — clear all statistics
 *
 * @param st  Stats context
 */
void plc_stats_reset(plc_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PLC_STATS_H */
