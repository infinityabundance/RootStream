/*
 * ladder_builder.c — ABR bitrate ladder builder
 */

#include "ladder_builder.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Standard output heights (descending) */
static const uint16_t std_heights[] = { 2160, 1440, 1080, 720, 480, 360, 240 };
static const int      n_std_heights = (int)(sizeof(std_heights)/sizeof(std_heights[0]));

/* Pick the nearest standard height ≤ max_height */
static uint16_t pick_height(uint16_t max_height, uint32_t bps, uint32_t max_bps) {
    /* Scale height proportionally then snap to nearest standard */
    double ratio = (max_bps > 0) ? sqrt((double)bps / (double)max_bps) : 1.0;
    uint16_t target = (uint16_t)(max_height * ratio);
    uint16_t best   = std_heights[n_std_heights - 1];
    for (int i = 0; i < n_std_heights; i++) {
        if (std_heights[i] <= max_height && std_heights[i] <= target) {
            best = std_heights[i];
            break;
        }
        if (std_heights[i] <= target) { best = std_heights[i]; break; }
    }
    if (best == 0) best = std_heights[n_std_heights - 1];
    return best;
}

/* 16:9 width from height */
static uint16_t height_to_width(uint16_t h) {
    return (uint16_t)((h * 16u) / 9u);
}

int ladder_build(const ladder_params_t *p,
                   ladder_rung_t         *rungs,
                   int                   *n_out) {
    if (!p || !rungs || !n_out) return -1;
    if (p->max_bps == 0 || p->min_bps == 0 || p->max_bps < p->min_bps) return -1;
    if (p->step_ratio <= 0.0f || p->step_ratio >= 1.0f) return -1;
    if (p->max_height == 0 || p->max_fps <= 0.0f) return -1;

    int    n   = 0;
    float  fps = p->max_fps;
    uint32_t bps = p->max_bps;

    while (bps >= p->min_bps && n < LADDER_MAX_RUNGS) {
        uint16_t h = pick_height(p->max_height, bps, p->max_bps);
        uint16_t w = height_to_width(h);

        float rung_fps = fps;
        if (p->fps_reduce_threshold > 0 && bps < p->fps_reduce_threshold)
            rung_fps = fps / 2.0f;

        lr_init(&rungs[n], bps, w, h, rung_fps);
        n++;

        uint32_t next_bps = (uint32_t)((double)bps * p->step_ratio);
        if (next_bps >= bps) break; /* safety: ensure strictly decreasing */
        bps = next_bps;
    }

    /* Sort ascending by bitrate */
    qsort(rungs, (size_t)n, sizeof(ladder_rung_t), lr_compare);
    *n_out = n;
    return 0;
}
