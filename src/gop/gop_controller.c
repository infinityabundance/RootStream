/*
 * gop_controller.c — Adaptive GOP controller implementation
 */

#include "gop_controller.h"

#include <stdlib.h>
#include <string.h>

struct gop_controller_s {
    gop_policy_t policy;
    int frames_since_idr;
};

gop_controller_t *gop_controller_create(const gop_policy_t *policy) {
    if (!policy || gop_policy_validate(policy) != 0)
        return NULL;
    gop_controller_t *gc = calloc(1, sizeof(*gc));
    if (!gc)
        return NULL;
    gc->policy = *policy;
    gc->frames_since_idr = 0;
    return gc;
}

void gop_controller_destroy(gop_controller_t *gc) {
    free(gc);
}

int gop_controller_update_policy(gop_controller_t *gc, const gop_policy_t *policy) {
    if (!gc || !policy || gop_policy_validate(policy) != 0)
        return -1;
    gc->policy = *policy;
    return 0;
}

int gop_controller_frames_since_idr(const gop_controller_t *gc) {
    return gc ? gc->frames_since_idr : 0;
}

void gop_controller_force_idr(gop_controller_t *gc) {
    if (gc)
        gc->frames_since_idr = 0;
}

gop_decision_t gop_controller_next_frame(gop_controller_t *gc, float scene_score, uint64_t rtt_us,
                                         float loss, gop_reason_t *reason_out) {
    if (!gc) {
        if (reason_out)
            *reason_out = GOP_REASON_NONE;
        return GOP_DECISION_P_FRAME;
    }

    gc->frames_since_idr++;
    const gop_policy_t *p = &gc->policy;

    /* Rule 2: maximum interval */
    if (gc->frames_since_idr >= p->max_gop_frames) {
        gc->frames_since_idr = 0;
        if (reason_out)
            *reason_out = GOP_REASON_NATURAL;
        return GOP_DECISION_IDR;
    }

    /* Rules 1+: cooldown — no forced IDRs within min_gop_frames */
    if (gc->frames_since_idr <= p->min_gop_frames) {
        if (reason_out)
            *reason_out = GOP_REASON_NONE;
        return GOP_DECISION_P_FRAME;
    }

    /* Rule 3: scene change */
    if (scene_score >= p->scene_change_threshold) {
        gc->frames_since_idr = 0;
        if (reason_out)
            *reason_out = GOP_REASON_SCENE_CHANGE;
        return GOP_DECISION_IDR;
    }

    /* Rule 4: loss recovery (only when RTT is below threshold) */
    if (loss >= p->loss_threshold && rtt_us < p->rtt_threshold_us) {
        gc->frames_since_idr = 0;
        if (reason_out)
            *reason_out = GOP_REASON_LOSS_RECOVERY;
        return GOP_DECISION_IDR;
    }

    if (reason_out)
        *reason_out = GOP_REASON_NONE;
    return GOP_DECISION_P_FRAME;
}

const char *gop_decision_name(gop_decision_t d) {
    switch (d) {
        case GOP_DECISION_P_FRAME:
            return "P_FRAME";
        case GOP_DECISION_IDR:
            return "IDR";
        default:
            return "UNKNOWN";
    }
}

const char *gop_reason_name(gop_reason_t r) {
    switch (r) {
        case GOP_REASON_NATURAL:
            return "NATURAL";
        case GOP_REASON_SCENE_CHANGE:
            return "SCENE_CHANGE";
        case GOP_REASON_LOSS_RECOVERY:
            return "LOSS_RECOVERY";
        case GOP_REASON_NONE:
            return "NONE";
        default:
            return "UNKNOWN";
    }
}
