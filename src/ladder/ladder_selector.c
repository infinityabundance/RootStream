/*
 * ladder_selector.c — ABR rung selector
 */

#include "ladder_selector.h"

int ladder_select(const ladder_rung_t *rungs, int n, uint32_t estimated_bw, float margin) {
    if (!rungs || n <= 0)
        return 0;
    if (margin < 0.0f)
        margin = 0.0f;
    if (margin > 1.0f)
        margin = 0.99f;

    double budget = (double)estimated_bw * (1.0 - (double)margin);
    int best = 0; /* always return at least the lowest rung */

    for (int i = 0; i < n; i++) {
        if ((double)rungs[i].bitrate_bps <= budget)
            best = i;
    }
    return best;
}
