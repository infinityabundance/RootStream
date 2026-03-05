/*
 * cs_stats.c — Clock sync statistics implementation
 */

#include "cs_stats.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

struct cs_stats_s {
    uint64_t sample_count;
    int64_t  min_offset_us;
    int64_t  max_offset_us;
    double   sum_offset_us;
    int64_t  min_rtt_us;
    int64_t  max_rtt_us;
    double   sum_rtt_us;
};

cs_stats_t *cs_stats_create(void) {
    cs_stats_t *st = calloc(1, sizeof(*st));
    if (st) {
        st->min_offset_us = INT64_MAX;
        st->max_offset_us = INT64_MIN;
        st->min_rtt_us    = INT64_MAX;
        st->max_rtt_us    = INT64_MIN;
    }
    return st;
}

void cs_stats_destroy(cs_stats_t *st) { free(st); }

void cs_stats_reset(cs_stats_t *st) {
    if (!st) return;
    memset(st, 0, sizeof(*st));
    st->min_offset_us = INT64_MAX;
    st->max_offset_us = INT64_MIN;
    st->min_rtt_us    = INT64_MAX;
    st->max_rtt_us    = INT64_MIN;
}

int cs_stats_record(cs_stats_t *st, int64_t offset_us, int64_t rtt_us) {
    if (!st) return -1;
    st->sample_count++;
    st->sum_offset_us += (double)offset_us;
    st->sum_rtt_us    += (double)rtt_us;
    if (offset_us < st->min_offset_us) st->min_offset_us = offset_us;
    if (offset_us > st->max_offset_us) st->max_offset_us = offset_us;
    if (rtt_us    < st->min_rtt_us)    st->min_rtt_us    = rtt_us;
    if (rtt_us    > st->max_rtt_us)    st->max_rtt_us    = rtt_us;
    return 0;
}

int cs_stats_snapshot(const cs_stats_t *st, cs_stats_snapshot_t *out) {
    if (!st || !out) return -1;
    out->sample_count  = st->sample_count;
    out->min_offset_us = (st->sample_count > 0) ? st->min_offset_us : 0;
    out->max_offset_us = (st->sample_count > 0) ? st->max_offset_us : 0;
    out->avg_offset_us = (st->sample_count > 0) ?
        st->sum_offset_us / (double)st->sample_count : 0.0;
    out->min_rtt_us    = (st->sample_count > 0) ? st->min_rtt_us : 0;
    out->max_rtt_us    = (st->sample_count > 0) ? st->max_rtt_us : 0;
    out->avg_rtt_us    = (st->sample_count > 0) ?
        st->sum_rtt_us / (double)st->sample_count : 0.0;
    out->converged     = (st->sample_count >= CS_CONVERGENCE_SAMPLES);
    return 0;
}
