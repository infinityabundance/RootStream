/*
 * frc_stats.c — Frame rate controller statistics implementation
 *
 * Actual FPS is computed as a Welford running mean of the instantaneous
 * 1-second rate, re-measured each time a presented frame is recorded.
 */

#include "frc_stats.h"

#include <stdlib.h>
#include <string.h>

struct frc_stats_s {
    uint64_t frames_presented;
    uint64_t frames_dropped;
    uint64_t frames_duplicated;

    /* For FPS estimation: count presented frames in current window */
    uint64_t window_start_ns;
    uint64_t window_present_count;
    double   actual_fps;
};

frc_stats_t *frc_stats_create(void) {
    return calloc(1, sizeof(frc_stats_t));
}

void frc_stats_destroy(frc_stats_t *st) {
    free(st);
}

void frc_stats_reset(frc_stats_t *st) {
    if (st) memset(st, 0, sizeof(*st));
}

int frc_stats_record(frc_stats_t *st,
                      int          presented,
                      int          dropped,
                      int          duplicated,
                      uint64_t     now_ns) {
    if (!st) return -1;

    if (presented) {
        st->frames_presented++;
        st->window_present_count++;

        /* Update FPS estimate once per second */
        if (st->window_start_ns == 0) {
            st->window_start_ns = now_ns;
        } else {
            uint64_t elapsed = now_ns - st->window_start_ns;
            if (elapsed >= 1000000000ULL) {
                double fps = (double)st->window_present_count /
                             ((double)elapsed / 1e9);
                /* EWMA update */
                if (st->actual_fps == 0.0) {
                    st->actual_fps = fps;
                } else {
                    st->actual_fps = 0.125 * fps + 0.875 * st->actual_fps;
                }
                st->window_start_ns      = now_ns;
                st->window_present_count = 0;
            }
        }
    }
    if (dropped)    st->frames_dropped++;
    if (duplicated) st->frames_duplicated++;
    return 0;
}

int frc_stats_snapshot(const frc_stats_t *st, frc_stats_snapshot_t *out) {
    if (!st || !out) return -1;
    out->frames_presented  = st->frames_presented;
    out->frames_dropped    = st->frames_dropped;
    out->frames_duplicated = st->frames_duplicated;
    out->actual_fps        = st->actual_fps;
    return 0;
}
