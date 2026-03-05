/*
 * frc_pacer.c — Frame rate controller pacer implementation
 *
 * Token accumulator model:
 *   Every tick we compute elapsed ns since last tick; tokens +=
 *   elapsed / frame_interval_ns (so tokens accumulate at the target rate).
 *   - tokens >= 1.0 → present a frame (tokens -= 1.0)
 *   - tokens < 0.0  → duplicate (we're behind; tokens += 1.0)
 *   - else          → drop (we're ahead; tokens stays)
 */

#include "frc_pacer.h"

#include <stdlib.h>
#include <math.h>

struct frc_pacer_s {
    double   target_fps;
    double   frame_interval_ns;  /* = 1e9 / fps */
    double   tokens;
    uint64_t last_ns;
};

frc_pacer_t *frc_pacer_create(double target_fps, uint64_t now_ns) {
    if (target_fps <= 0.0 || target_fps > 1000.0) return NULL;
    frc_pacer_t *p = malloc(sizeof(*p));
    if (!p) return NULL;
    p->target_fps         = target_fps;
    p->frame_interval_ns  = 1e9 / target_fps;
    p->tokens             = 1.0; /* First frame is always presented */
    p->last_ns            = now_ns;
    return p;
}

void frc_pacer_destroy(frc_pacer_t *p) {
    free(p);
}

int frc_pacer_set_fps(frc_pacer_t *p, double target_fps) {
    if (!p || target_fps <= 0.0 || target_fps > 1000.0) return -1;
    p->target_fps        = target_fps;
    p->frame_interval_ns = 1e9 / target_fps;
    return 0;
}

double frc_pacer_target_fps(const frc_pacer_t *p) {
    return p ? p->target_fps : 0.0;
}

frc_action_t frc_pacer_tick(frc_pacer_t *p, uint64_t now_ns) {
    if (!p) return FRC_ACTION_DROP;

    if (now_ns > p->last_ns) {
        double elapsed = (double)(now_ns - p->last_ns);
        p->tokens += elapsed / p->frame_interval_ns;
        p->last_ns = now_ns;
        /* Cap tokens to avoid unbounded accumulation after long pauses */
        if (p->tokens > 2.0) p->tokens = 2.0;
    }

    if (p->tokens >= 1.0) {
        p->tokens -= 1.0;
        return FRC_ACTION_PRESENT;
    } else if (p->tokens < 0.0) {
        p->tokens += 1.0;
        return FRC_ACTION_DUPLICATE;
    } else {
        return FRC_ACTION_DROP;
    }
}

const char *frc_action_name(frc_action_t a) {
    switch (a) {
    case FRC_ACTION_PRESENT:   return "present";
    case FRC_ACTION_DROP:      return "drop";
    case FRC_ACTION_DUPLICATE: return "duplicate";
    default:                    return "unknown";
    }
}
