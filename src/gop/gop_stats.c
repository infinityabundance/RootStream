/*
 * gop_stats.c — GOP statistics implementation
 */

#include "gop_stats.h"

#include <stdlib.h>
#include <string.h>

struct gop_stats_s {
    uint64_t total_frames;
    uint64_t idr_natural;
    uint64_t idr_scene_change;
    uint64_t idr_loss_recovery;
    /* For avg GOP length: accumulate frames between IDRs */
    uint64_t gop_length_sum;
    uint64_t gop_count;
    uint64_t frames_in_current_gop;
};

gop_stats_t *gop_stats_create(void) {
    return calloc(1, sizeof(gop_stats_t));
}

void gop_stats_destroy(gop_stats_t *st) { free(st); }

void gop_stats_reset(gop_stats_t *st) {
    if (st) memset(st, 0, sizeof(*st));
}

int gop_stats_record(gop_stats_t *st, int is_idr, gop_reason_t reason) {
    if (!st) return -1;
    st->total_frames++;
    st->frames_in_current_gop++;

    if (is_idr) {
        switch (reason) {
        case GOP_REASON_NATURAL:       st->idr_natural++;       break;
        case GOP_REASON_SCENE_CHANGE:  st->idr_scene_change++;  break;
        case GOP_REASON_LOSS_RECOVERY: st->idr_loss_recovery++; break;
        default: break;
        }
        /* Complete the current GOP */
        if (st->frames_in_current_gop > 0) {
            st->gop_length_sum      += st->frames_in_current_gop;
            st->gop_count++;
        }
        st->frames_in_current_gop = 0;
    }
    return 0;
}

int gop_stats_snapshot(const gop_stats_t *st, gop_stats_snapshot_t *out) {
    if (!st || !out) return -1;
    out->total_frames      = st->total_frames;
    out->idr_natural       = st->idr_natural;
    out->idr_scene_change  = st->idr_scene_change;
    out->idr_loss_recovery = st->idr_loss_recovery;
    out->total_idrs        = st->idr_natural + st->idr_scene_change +
                             st->idr_loss_recovery;
    out->avg_gop_length    = (st->gop_count > 0) ?
        (double)st->gop_length_sum / (double)st->gop_count : 0.0;
    return 0;
}
