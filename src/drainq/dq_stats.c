/*
 * dq_stats.c — Drain queue statistics
 */

#include "dq_stats.h"
#include <stdlib.h>
#include <string.h>

struct dq_stats_s {
    uint64_t enqueued;
    uint64_t drained;
    uint64_t dropped;
    int      peak;
};

dq_stats_t *dq_stats_create(void) { return calloc(1, sizeof(dq_stats_t)); }
void        dq_stats_destroy(dq_stats_t *st) { free(st); }
void        dq_stats_reset(dq_stats_t *st) { if (st) memset(st, 0, sizeof(*st)); }

int dq_stats_record_enqueue(dq_stats_t *st, int cur_depth) {
    if (!st) return -1;
    st->enqueued++;
    if (cur_depth > st->peak) st->peak = cur_depth;
    return 0;
}
int dq_stats_record_drain(dq_stats_t *st) {
    if (!st) return -1;
    st->drained++;
    return 0;
}
int dq_stats_record_drop(dq_stats_t *st) {
    if (!st) return -1;
    st->dropped++;
    return 0;
}

int dq_stats_snapshot(const dq_stats_t *st, dq_stats_snapshot_t *out) {
    if (!st || !out) return -1;
    out->enqueued = st->enqueued;
    out->drained  = st->drained;
    out->dropped  = st->dropped;
    out->peak     = st->peak;
    return 0;
}
