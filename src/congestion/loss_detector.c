/*
 * loss_detector.c — Sliding-window packet loss detector
 */

#include "loss_detector.h"

#include <stdlib.h>
#include <string.h>

struct loss_detector_s {
    uint8_t window[LOSS_WINDOW_SIZE]; /* 0=received, 1=lost */
    int     head;
    int     count;
    int     lost_count;
    double  threshold;
    bool    congested;
};

loss_detector_t *loss_detector_create(double threshold) {
    if (threshold < 0.0 || threshold > 1.0) return NULL;
    loss_detector_t *d = calloc(1, sizeof(*d));
    if (!d) return NULL;
    d->threshold = threshold;
    return d;
}

void loss_detector_destroy(loss_detector_t *d) { free(d); }

void loss_detector_reset(loss_detector_t *d) {
    if (!d) return;
    double t = d->threshold;
    memset(d, 0, sizeof(*d));
    d->threshold = t;
}

int loss_detector_set_threshold(loss_detector_t *d, double threshold) {
    if (!d || threshold < 0.0 || threshold > 1.0) return -1;
    d->threshold = threshold;
    return 0;
}

loss_signal_t loss_detector_record(loss_detector_t *d, loss_outcome_t outcome) {
    if (!d) return LOSS_SIGNAL_NONE;

    /* Evict oldest entry if window full */
    if (d->count >= LOSS_WINDOW_SIZE) {
        int oldest = (d->head - d->count + LOSS_WINDOW_SIZE * 2) % LOSS_WINDOW_SIZE;
        if (d->window[oldest]) d->lost_count--;
        d->count--;
    }

    d->window[d->head] = (uint8_t)(outcome == LOSS_OUTCOME_LOST ? 1 : 0);
    if (outcome == LOSS_OUTCOME_LOST) d->lost_count++;
    d->head = (d->head + 1) % LOSS_WINDOW_SIZE;
    d->count++;

    double frac = (d->count > 0) ? (double)d->lost_count / (double)d->count : 0.0;
    d->congested = (frac > d->threshold);
    return d->congested ? LOSS_SIGNAL_CONGESTED : LOSS_SIGNAL_NONE;
}

double loss_detector_loss_fraction(const loss_detector_t *d) {
    if (!d || d->count == 0) return 0.0;
    return (double)d->lost_count / (double)d->count;
}

bool loss_detector_is_congested(const loss_detector_t *d) {
    return d && d->congested;
}
