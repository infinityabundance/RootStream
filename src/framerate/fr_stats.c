/*
 * fr_stats.c — Frame rate statistics implementation
 */

#include "fr_stats.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

struct fr_stats_s {
    uint64_t frame_count;
    uint64_t drop_count;
    double   sum_interval_us;
    uint64_t min_interval_us;
    uint64_t max_interval_us;
};

fr_stats_t *fr_stats_create(void) {
    fr_stats_t *st = calloc(1, sizeof(*st));
    if (st) {
        st->min_interval_us = UINT64_MAX;
    }
    return st;
}

void fr_stats_destroy(fr_stats_t *st) { free(st); }

void fr_stats_reset(fr_stats_t *st) {
    if (!st) return;
    memset(st, 0, sizeof(*st));
    st->min_interval_us = UINT64_MAX;
}

int fr_stats_record_frame(fr_stats_t *st, uint64_t interval_us) {
    if (!st) return -1;
    st->frame_count++;
    st->sum_interval_us += (double)interval_us;
    if (interval_us < st->min_interval_us) st->min_interval_us = interval_us;
    if (interval_us > st->max_interval_us) st->max_interval_us = interval_us;
    return 0;
}

int fr_stats_record_drop(fr_stats_t *st) {
    if (!st) return -1;
    st->drop_count++;
    return 0;
}

int fr_stats_snapshot(const fr_stats_t *st, fr_stats_snapshot_t *out) {
    if (!st || !out) return -1;
    out->frame_count   = st->frame_count;
    out->drop_count    = st->drop_count;
    out->avg_interval_us = (st->frame_count > 0)
        ? st->sum_interval_us / (double)st->frame_count : 0.0;
    out->min_interval_us = (st->frame_count > 0) ? st->min_interval_us : 0;
    out->max_interval_us = st->max_interval_us;
    return 0;
}
