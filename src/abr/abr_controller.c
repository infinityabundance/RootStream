/*
 * abr_controller.c — ABR decision engine implementation
 */

#include "abr_controller.h"

#include <stdlib.h>
#include <string.h>

struct abr_controller_s {
    abr_estimator_t *estimator; /* borrowed */
    abr_ladder_t *ladder;       /* borrowed */
    int current_idx;
    int stable_ticks; /* consecutive ticks at same/higher level */
    int forced_idx;   /* -1 = auto */
};

abr_controller_t *abr_controller_create(abr_estimator_t *estimator, abr_ladder_t *ladder) {
    if (!estimator || !ladder)
        return NULL;
    abr_controller_t *c = calloc(1, sizeof(*c));
    if (!c)
        return NULL;
    c->estimator = estimator;
    c->ladder = ladder;
    c->forced_idx = -1;
    /* Start at lowest quality level */
    c->current_idx = 0;
    return c;
}

void abr_controller_destroy(abr_controller_t *ctrl) {
    free(ctrl);
}

int abr_controller_current_level(const abr_controller_t *ctrl) {
    return ctrl ? ctrl->current_idx : -1;
}

int abr_controller_force_level(abr_controller_t *ctrl, int level_idx) {
    if (!ctrl)
        return -1;
    if (level_idx < -1 || level_idx >= abr_ladder_count(ctrl->ladder))
        return -1;
    ctrl->forced_idx = level_idx;
    if (level_idx >= 0)
        ctrl->current_idx = level_idx;
    return 0;
}

int abr_controller_tick(abr_controller_t *ctrl, abr_decision_t *out) {
    if (!ctrl || !out)
        return -1;

    int prev = ctrl->current_idx;

    if (ctrl->forced_idx >= 0) {
        /* Manual override */
        out->new_level_idx = ctrl->forced_idx;
        out->level_changed = (ctrl->forced_idx != prev);
        out->is_downgrade = (ctrl->forced_idx < prev);
        ctrl->current_idx = ctrl->forced_idx;
        return 0;
    }

    if (!abr_estimator_is_ready(ctrl->estimator)) {
        /* Not enough samples yet — stay at lowest level */
        out->new_level_idx = 0;
        out->level_changed = (0 != prev);
        out->is_downgrade = (0 < prev);
        ctrl->current_idx = 0;
        return 0;
    }

    double bw = abr_estimator_get(ctrl->estimator);
    double budget = bw * (double)ABR_SAFETY_MARGIN;
    int target = abr_ladder_select(ctrl->ladder, budget);
    if (target < 0)
        target = 0;

    int n = abr_ladder_count(ctrl->ladder);
    if (target < 0)
        target = 0;
    if (target >= n)
        target = n - 1;

    int new_idx;
    if (target < ctrl->current_idx) {
        /* Immediate downgrade */
        new_idx = target;
        ctrl->stable_ticks = 0;
    } else if (target > ctrl->current_idx) {
        /* Upgrade only after holding stable for hold period */
        ctrl->stable_ticks++;
        if (ctrl->stable_ticks >= ABR_UPGRADE_HOLD_TICKS) {
            /* Upgrade by one step at a time */
            new_idx = ctrl->current_idx + 1;
            if (new_idx >= n)
                new_idx = n - 1;
            ctrl->stable_ticks = 0;
        } else {
            new_idx = ctrl->current_idx;
        }
    } else {
        /* At target level — count stable ticks */
        ctrl->stable_ticks++;
        new_idx = ctrl->current_idx;
    }

    out->new_level_idx = new_idx;
    out->level_changed = (new_idx != prev);
    out->is_downgrade = (new_idx < prev);
    ctrl->current_idx = new_idx;
    return 0;
}
