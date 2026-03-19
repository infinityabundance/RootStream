/*
 * fr_target.c — Target FPS tracker
 */

#include "fr_target.h"

#include <string.h>

int fr_target_init(fr_target_t *t, double target_fps) {
    if (!t || target_fps <= 0.0)
        return -1;
    memset(t, 0, sizeof(*t));
    t->target_fps = target_fps;
    /* Initialise avg_interval to the ideal interval */
    t->avg_interval_us = 1e6 / target_fps;
    t->actual_fps = target_fps;
    return 0;
}

void fr_target_reset(fr_target_t *t) {
    if (!t)
        return;
    double fps = t->target_fps;
    memset(t, 0, sizeof(*t));
    t->target_fps = fps;
    t->avg_interval_us = (fps > 0.0) ? 1e6 / fps : 0.0;
    t->actual_fps = fps;
}

int fr_target_mark(fr_target_t *t, uint64_t now_us) {
    if (!t)
        return -1;
    t->frame_count++;

    if (!t->initialised) {
        t->last_mark_us = now_us;
        t->initialised = 1;
        return 0;
    }

    if (now_us <= t->last_mark_us) {
        t->last_mark_us = now_us;
        return 0;
    }

    double interval = (double)(now_us - t->last_mark_us);
    t->avg_interval_us =
        (1.0 - FR_TARGET_EWMA_ALPHA) * t->avg_interval_us + FR_TARGET_EWMA_ALPHA * interval;
    if (t->avg_interval_us > 0.0)
        t->actual_fps = 1e6 / t->avg_interval_us;

    t->last_mark_us = now_us;
    return 0;
}
