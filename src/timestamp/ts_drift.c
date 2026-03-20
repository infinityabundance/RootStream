/*
 * ts_drift.c — Stream clock drift estimator
 */

#include "ts_drift.h"

#include <math.h>
#include <string.h>

int ts_drift_init(ts_drift_t *d) {
    if (!d)
        return -1;
    memset(d, 0, sizeof(*d));
    return 0;
}

void ts_drift_reset(ts_drift_t *d) {
    if (d)
        memset(d, 0, sizeof(*d));
}

int ts_drift_update(ts_drift_t *d, int64_t observed_us, int64_t expected_us) {
    if (!d)
        return -1;

    double error = (double)(observed_us - expected_us);

    if (!d->initialised) {
        d->ewma_error_us = error;
        d->initialised = 1;
    } else {
        d->ewma_error_us =
            (1.0 - TS_DRIFT_EWMA_ALPHA) * d->ewma_error_us + TS_DRIFT_EWMA_ALPHA * error;
    }

    /* Estimate drift in µs/second using the time elapsed since last sample */
    if (d->sample_count > 0 && observed_us > (int64_t)d->last_obs_us) {
        double elapsed_s = (double)(observed_us - (int64_t)d->last_obs_us) / 1e6;
        if (elapsed_s > 0.0)
            d->drift_us_per_sec = d->ewma_error_us / elapsed_s;
    }

    d->last_obs_us = (uint64_t)observed_us;
    d->sample_count++;
    return 0;
}
