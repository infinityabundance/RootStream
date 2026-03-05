/*
 * ts_drift.h — Stream clock drift estimator
 *
 * Estimates the drift between a stream clock and the wall clock.
 * At each measurement point the caller provides an observed wall-clock
 * time and the expected wall-clock time (derived from ts_map_pts_to_us).
 * The difference (observed − expected) is smoothed via an EWMA to
 * produce `drift_us_per_sec`.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_TS_DRIFT_H
#define ROOTSTREAM_TS_DRIFT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TS_DRIFT_EWMA_ALPHA  0.1   /**< EWMA smoothing factor */

/** Clock drift estimator */
typedef struct {
    double   ewma_error_us;     /**< Smoothed (observed-expected) µs */
    double   drift_us_per_sec;  /**< Estimated drift in µs/second */
    uint64_t last_obs_us;       /**< Last observed wall-clock µs */
    uint64_t sample_count;      /**< Total measurements */
    int      initialised;
} ts_drift_t;

/**
 * ts_drift_init — initialise estimator
 *
 * @param d  Estimator
 * @return   0 on success, -1 on NULL
 */
int ts_drift_init(ts_drift_t *d);

/**
 * ts_drift_update — record a new measurement
 *
 * @param d           Estimator
 * @param observed_us Actual wall-clock µs
 * @param expected_us Expected wall-clock µs (from ts_map)
 * @return            0 on success, -1 on NULL
 */
int ts_drift_update(ts_drift_t *d, int64_t observed_us, int64_t expected_us);

/**
 * ts_drift_reset — clear all state
 */
void ts_drift_reset(ts_drift_t *d);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_TS_DRIFT_H */
