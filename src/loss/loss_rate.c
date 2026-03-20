/*
 * loss_rate.c — Running loss rate estimator
 */

#include "loss_rate.h"

#include <string.h>

int lr_rate_init(loss_rate_t *lr) {
    if (!lr)
        return -1;
    memset(lr, 0, sizeof(*lr));
    lw_init(&lr->window);
    return 0;
}

void lr_rate_reset(loss_rate_t *lr) {
    if (lr) {
        lw_reset(&lr->window);
        lr->ewma_loss_rate = 0.0;
        lr->ready = false;
    }
}

int lr_rate_receive(loss_rate_t *lr, uint16_t seq) {
    if (!lr)
        return -1;
    lw_receive(&lr->window, seq);
    double instant = lw_loss_rate(&lr->window);
    if (!lr->ready) {
        lr->ewma_loss_rate = instant;
        lr->ready = true;
    } else {
        lr->ewma_loss_rate =
            (1.0 - LOSS_RATE_EWMA_ALPHA) * lr->ewma_loss_rate + LOSS_RATE_EWMA_ALPHA * instant;
    }
    return 0;
}

double lr_rate_get(const loss_rate_t *lr) {
    return lr ? lw_loss_rate(&lr->window) : 0.0;
}

double lr_rate_ewma(const loss_rate_t *lr) {
    return lr ? lr->ewma_loss_rate : 0.0;
}
