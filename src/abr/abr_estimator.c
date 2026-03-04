/*
 * abr_estimator.c — EWMA bandwidth estimator implementation
 */

#include "abr_estimator.h"

#include <stdlib.h>
#include <string.h>

struct abr_estimator_s {
    float  alpha;
    double ewma;
    size_t count;
};

abr_estimator_t *abr_estimator_create(float alpha) {
    if (alpha <= 0.0f || alpha >= 1.0f) return NULL;
    abr_estimator_t *e = calloc(1, sizeof(*e));
    if (!e) return NULL;
    e->alpha = alpha;
    return e;
}

void abr_estimator_destroy(abr_estimator_t *est) {
    free(est);
}

int abr_estimator_update(abr_estimator_t *est, double bps_sample) {
    if (!est) return -1;
    if (est->count == 0) {
        est->ewma = bps_sample; /* Initialise with first sample */
    } else {
        est->ewma = (double)est->alpha * bps_sample +
                    (1.0 - (double)est->alpha) * est->ewma;
    }
    est->count++;
    return 0;
}

double abr_estimator_get(const abr_estimator_t *est) {
    return est ? est->ewma : 0.0;
}

bool abr_estimator_is_ready(const abr_estimator_t *est) {
    return est ? (est->count >= ABR_ESTIMATOR_MIN_SAMPLES) : false;
}

void abr_estimator_reset(abr_estimator_t *est) {
    if (est) {
        est->ewma  = 0.0;
        est->count = 0;
    }
}

size_t abr_estimator_sample_count(const abr_estimator_t *est) {
    return est ? est->count : 0;
}
