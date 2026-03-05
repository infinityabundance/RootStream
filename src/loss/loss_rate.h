/*
 * loss_rate.h — Running packet loss rate estimator
 *
 * Wraps loss_window_t with convenience accessors and a secondary
 * exponentially-weighted moving average (EWMA) loss rate for smoother
 * control-loop feedback.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_LOSS_RATE_H
#define ROOTSTREAM_LOSS_RATE_H

#include "loss_window.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOSS_RATE_EWMA_ALPHA  0.125   /**< EWMA smoothing factor */

/** Loss rate estimator */
typedef struct {
    loss_window_t window;          /**< Sliding packet window */
    double        ewma_loss_rate;  /**< Smoothed loss rate [0,1] */
    bool          ready;           /**< True once first packet received */
} loss_rate_t;

/**
 * lr_rate_init — initialise estimator
 *
 * @param lr  Estimator to initialise
 * @return    0 on success, -1 on NULL
 */
int lr_rate_init(loss_rate_t *lr);

/**
 * lr_rate_receive — mark a packet received and update EWMA
 *
 * @param lr   Estimator
 * @param seq  Received sequence number
 * @return     0 on success, -1 on NULL
 */
int lr_rate_receive(loss_rate_t *lr, uint16_t seq);

/**
 * lr_rate_get — current instantaneous loss rate from window
 *
 * @param lr  Estimator
 * @return    Loss rate in [0, 1]
 */
double lr_rate_get(const loss_rate_t *lr);

/**
 * lr_rate_ewma — smoothed EWMA loss rate
 *
 * @param lr  Estimator
 * @return    EWMA loss rate in [0, 1]
 */
double lr_rate_ewma(const loss_rate_t *lr);

/**
 * lr_rate_reset — clear state
 *
 * @param lr  Estimator
 */
void lr_rate_reset(loss_rate_t *lr);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_LOSS_RATE_H */
