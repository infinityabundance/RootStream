/*
 * fr_limiter.c — Token-bucket frame rate limiter
 */

#include "fr_limiter.h"

#include <string.h>

int fr_limiter_init(fr_limiter_t *l, double target_fps) {
    if (!l || target_fps <= 0.0)
        return -1;
    l->target_fps = target_fps;
    l->tokens = 0.0;
    l->max_burst = FR_MAX_BURST;
    return 0;
}

void fr_limiter_reset(fr_limiter_t *l) {
    if (l)
        l->tokens = 0.0;
}

int fr_limiter_set_fps(fr_limiter_t *l, double fps) {
    if (!l || fps <= 0.0)
        return -1;
    l->target_fps = fps;
    return 0;
}

int fr_limiter_tick(fr_limiter_t *l, uint64_t elapsed_us) {
    if (!l || l->target_fps <= 0.0)
        return 0;

    /* Accumulate tokens: elapsed seconds × target fps */
    double earned = ((double)elapsed_us / 1e6) * l->target_fps;
    l->tokens += earned;
    if (l->tokens > (double)l->max_burst)
        l->tokens = (double)l->max_burst;

    int frames = (int)l->tokens;
    if (frames > 0)
        l->tokens -= (double)frames;
    return frames;
}
