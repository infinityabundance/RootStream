/*
 * abr_estimator.h — EWMA bandwidth estimator for adaptive bitrate control
 *
 * Maintains an exponentially-weighted moving average of the observed
 * delivery bandwidth, updated on each acknowledgement of a segment or
 * packet group.
 *
 * Thread-safety: NOT thread-safe; protect with external lock for
 * multi-threaded use.
 */

#ifndef ROOTSTREAM_ABR_ESTIMATOR_H
#define ROOTSTREAM_ABR_ESTIMATOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** EWMA smoothing factor (α): higher = more responsive, lower = smoother */
#define ABR_EWMA_ALPHA_DEFAULT  0.125f

/** Minimum number of samples before estimate is considered valid */
#define ABR_ESTIMATOR_MIN_SAMPLES  3

/** Opaque bandwidth estimator */
typedef struct abr_estimator_s abr_estimator_t;

/**
 * abr_estimator_create — allocate estimator
 *
 * @param alpha  EWMA smoothing factor (0 < alpha < 1); use
 *               ABR_EWMA_ALPHA_DEFAULT for typical live streaming
 * @return       Non-NULL handle, or NULL on OOM / bad alpha
 */
abr_estimator_t *abr_estimator_create(float alpha);

/**
 * abr_estimator_destroy — free estimator
 *
 * @param est  Estimator to destroy
 */
void abr_estimator_destroy(abr_estimator_t *est);

/**
 * abr_estimator_update — feed a new observed bandwidth sample
 *
 * @param est        Estimator
 * @param bps_sample Observed bandwidth sample in bits per second
 * @return           0 on success, -1 on NULL args
 */
int abr_estimator_update(abr_estimator_t *est, double bps_sample);

/**
 * abr_estimator_get — retrieve current EWMA estimate
 *
 * @param est  Estimator
 * @return     Estimated bandwidth in bps, or 0.0 if no samples yet
 */
double abr_estimator_get(const abr_estimator_t *est);

/**
 * abr_estimator_is_ready — return true if >= MIN_SAMPLES have been fed
 *
 * @param est  Estimator
 * @return     true when estimate is reliable
 */
bool abr_estimator_is_ready(const abr_estimator_t *est);

/**
 * abr_estimator_reset — clear all samples and restart estimation
 *
 * @param est  Estimator
 */
void abr_estimator_reset(abr_estimator_t *est);

/**
 * abr_estimator_sample_count — number of samples ingested
 *
 * @param est  Estimator
 * @return     Count
 */
size_t abr_estimator_sample_count(const abr_estimator_t *est);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_ABR_ESTIMATOR_H */
