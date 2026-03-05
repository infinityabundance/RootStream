/*
 * loss_detector.h — Sliding-window packet loss detector
 *
 * Maintains a circular bitset of the last LOSS_WINDOW_SIZE packet
 * outcomes (1 = lost, 0 = received) and emits a congestion signal
 * when the window loss fraction exceeds a configurable threshold.
 *
 * Designed to integrate with the rtt_estimator: the caller feeds both
 * RTT samples and packet outcomes; the detector reports whether the
 * current path should be considered congested.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_LOSS_DETECTOR_H
#define ROOTSTREAM_LOSS_DETECTOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOSS_WINDOW_SIZE  128          /**< Sliding window depth (packets) */
#define LOSS_DEFAULT_THRESHOLD  0.05   /**< Default congestion threshold (5%) */

/** Packet outcome */
typedef enum {
    LOSS_OUTCOME_RECEIVED = 0,
    LOSS_OUTCOME_LOST     = 1,
} loss_outcome_t;

/** Congestion signal (returned by loss_detector_record) */
typedef enum {
    LOSS_SIGNAL_NONE      = 0,   /**< No congestion */
    LOSS_SIGNAL_CONGESTED = 1,   /**< Loss fraction exceeded threshold */
} loss_signal_t;

/** Opaque loss detector */
typedef struct loss_detector_s loss_detector_t;

/**
 * loss_detector_create — allocate detector
 *
 * @param threshold  Loss fraction [0.0, 1.0] that triggers CONGESTED
 * @return           Non-NULL handle, or NULL on error
 */
loss_detector_t *loss_detector_create(double threshold);

/**
 * loss_detector_destroy — free detector
 *
 * @param d  Detector to destroy
 */
void loss_detector_destroy(loss_detector_t *d);

/**
 * loss_detector_record — record one packet outcome
 *
 * @param d        Detector
 * @param outcome  RECEIVED or LOST
 * @return         LOSS_SIGNAL_NONE or LOSS_SIGNAL_CONGESTED
 */
loss_signal_t loss_detector_record(loss_detector_t *d, loss_outcome_t outcome);

/**
 * loss_detector_loss_fraction — current window loss fraction
 *
 * @param d  Detector
 * @return   Fraction [0.0, 1.0]
 */
double loss_detector_loss_fraction(const loss_detector_t *d);

/**
 * loss_detector_is_congested — return true if last record returned CONGESTED
 *
 * @param d  Detector
 * @return   true if congested
 */
bool loss_detector_is_congested(const loss_detector_t *d);

/**
 * loss_detector_reset — clear window and congestion state
 *
 * @param d  Detector
 */
void loss_detector_reset(loss_detector_t *d);

/**
 * loss_detector_set_threshold — update congestion threshold
 *
 * @param d          Detector
 * @param threshold  New threshold [0.0, 1.0]
 * @return           0 on success, -1 on invalid args
 */
int loss_detector_set_threshold(loss_detector_t *d, double threshold);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_LOSS_DETECTOR_H */
