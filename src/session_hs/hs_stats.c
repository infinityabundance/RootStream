/*
 * hs_stats.c — Handshake statistics implementation
 */

#include "hs_stats.h"

#include <float.h>
#include <stdlib.h>
#include <string.h>

struct hs_stats_s {
    uint64_t attempts;
    uint64_t successes;
    uint64_t failures;
    uint64_t timeouts;

    /* RTT accumulators */
    uint64_t pending_start_us; /* set by hs_stats_begin */
    double rtt_sum_us;
    double min_rtt_us;
    double max_rtt_us;
};

hs_stats_t *hs_stats_create(void) {
    hs_stats_t *st = calloc(1, sizeof(*st));
    if (st)
        st->min_rtt_us = DBL_MAX;
    return st;
}

void hs_stats_destroy(hs_stats_t *st) {
    free(st);
}

void hs_stats_reset(hs_stats_t *st) {
    if (!st)
        return;
    memset(st, 0, sizeof(*st));
    st->min_rtt_us = DBL_MAX;
}

int hs_stats_begin(hs_stats_t *st, uint64_t now_us) {
    if (!st)
        return -1;
    st->attempts++;
    st->pending_start_us = now_us;
    return 0;
}

int hs_stats_complete(hs_stats_t *st, uint64_t now_us) {
    if (!st)
        return -1;
    st->successes++;
    if (st->pending_start_us > 0 && now_us >= st->pending_start_us) {
        double rtt = (double)(now_us - st->pending_start_us);
        st->rtt_sum_us += rtt;
        if (rtt < st->min_rtt_us)
            st->min_rtt_us = rtt;
        if (rtt > st->max_rtt_us)
            st->max_rtt_us = rtt;
    }
    st->pending_start_us = 0;
    return 0;
}

int hs_stats_fail(hs_stats_t *st) {
    if (!st)
        return -1;
    st->failures++;
    st->pending_start_us = 0;
    return 0;
}

int hs_stats_timeout(hs_stats_t *st) {
    if (!st)
        return -1;
    st->timeouts++;
    st->pending_start_us = 0;
    return 0;
}

int hs_stats_snapshot(const hs_stats_t *st, hs_stats_snapshot_t *out) {
    if (!st || !out)
        return -1;
    out->attempts = st->attempts;
    out->successes = st->successes;
    out->failures = st->failures;
    out->timeouts = st->timeouts;
    out->avg_rtt_us = (st->successes > 0) ? (st->rtt_sum_us / (double)st->successes) : 0.0;
    out->min_rtt_us = (st->min_rtt_us == DBL_MAX) ? 0.0 : st->min_rtt_us;
    out->max_rtt_us = st->max_rtt_us;
    return 0;
}
