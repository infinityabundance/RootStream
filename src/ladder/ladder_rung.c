/*
 * ladder_rung.c — ABR ladder rung implementation
 */

#include "ladder_rung.h"

int lr_init(ladder_rung_t *r, uint32_t bps, uint16_t width, uint16_t height, float fps) {
    if (!r || bps == 0 || width == 0 || height == 0 || fps <= 0.0f)
        return -1;
    r->bitrate_bps = bps;
    r->width = width;
    r->height = height;
    r->fps = fps;
    return 0;
}

int lr_compare(const void *a, const void *b) {
    const ladder_rung_t *ra = (const ladder_rung_t *)a;
    const ladder_rung_t *rb = (const ladder_rung_t *)b;
    if (ra->bitrate_bps < rb->bitrate_bps)
        return -1;
    if (ra->bitrate_bps > rb->bitrate_bps)
        return 1;
    return 0;
}
