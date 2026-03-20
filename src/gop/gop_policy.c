/*
 * gop_policy.c — GOP policy helpers
 */

#include "gop_policy.h"

int gop_policy_default(gop_policy_t *p) {
    if (!p)
        return -1;
    p->min_gop_frames = GOP_DEFAULT_MIN_FRAMES;
    p->max_gop_frames = GOP_DEFAULT_MAX_FRAMES;
    p->scene_change_threshold = GOP_DEFAULT_SCENE_THRESHOLD;
    p->rtt_threshold_us = GOP_DEFAULT_RTT_THRESHOLD_US;
    p->loss_threshold = GOP_DEFAULT_LOSS_THRESHOLD;
    return 0;
}

int gop_policy_validate(const gop_policy_t *p) {
    if (!p)
        return -1;
    if (p->min_gop_frames < 1)
        return -1;
    if (p->max_gop_frames < p->min_gop_frames)
        return -1;
    if (p->scene_change_threshold < 0.0f || p->scene_change_threshold > 1.0f)
        return -1;
    if (p->loss_threshold < 0.0f || p->loss_threshold > 1.0f)
        return -1;
    return 0;
}
