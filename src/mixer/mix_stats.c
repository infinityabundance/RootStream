/*
 * mix_stats.c — Mixer statistics implementation
 */

#include "mix_stats.h"

#include <stdlib.h>
#include <string.h>
#include <float.h>

struct mix_stats_s {
    uint64_t mix_calls;
    uint64_t active_sources;
    uint64_t muted_sources;
    uint64_t underruns;
    double   latency_sum_us;
    double   min_latency_us;
    double   max_latency_us;
};

mix_stats_t *mix_stats_create(void) {
    mix_stats_t *st = calloc(1, sizeof(*st));
    if (st) st->min_latency_us = DBL_MAX;
    return st;
}

void mix_stats_destroy(mix_stats_t *st) { free(st); }

void mix_stats_reset(mix_stats_t *st) {
    if (!st) return;
    memset(st, 0, sizeof(*st));
    st->min_latency_us = DBL_MAX;
}

int mix_stats_record(mix_stats_t *st,
                     int          active_count,
                     int          muted_count,
                     uint64_t     latency_us) {
    if (!st) return -1;
    st->mix_calls++;
    st->active_sources += (uint64_t)(active_count > 0 ? active_count : 0);
    st->muted_sources  += (uint64_t)(muted_count  > 0 ? muted_count  : 0);
    if (active_count == 0) st->underruns++;

    if (latency_us > 0) {
        double lat = (double)latency_us;
        st->latency_sum_us += lat;
        if (lat < st->min_latency_us) st->min_latency_us = lat;
        if (lat > st->max_latency_us) st->max_latency_us = lat;
    }
    return 0;
}

int mix_stats_snapshot(const mix_stats_t *st, mix_stats_snapshot_t *out) {
    if (!st || !out) return -1;
    out->mix_calls      = st->mix_calls;
    out->active_sources = st->active_sources;
    out->muted_sources  = st->muted_sources;
    out->underruns      = st->underruns;
    out->avg_latency_us = (st->mix_calls > 0) ?
                          (st->latency_sum_us / (double)st->mix_calls) : 0.0;
    out->min_latency_us = (st->min_latency_us == DBL_MAX) ? 0.0 : st->min_latency_us;
    out->max_latency_us = st->max_latency_us;
    return 0;
}
