/*
 * reorder_stats.c — Reorder buffer statistics implementation
 */

#include "reorder_stats.h"

#include <stdlib.h>
#include <string.h>

struct reorder_stats_s {
    uint64_t packets_inserted;
    uint64_t packets_delivered;
    uint64_t late_flushes;
    uint64_t discards;
    int      max_depth;
};

reorder_stats_t *reorder_stats_create(void) {
    return calloc(1, sizeof(reorder_stats_t));
}

void reorder_stats_destroy(reorder_stats_t *st) { free(st); }

void reorder_stats_reset(reorder_stats_t *st) {
    if (st) memset(st, 0, sizeof(*st));
}

int reorder_stats_record_insert(reorder_stats_t *st, int success, int depth) {
    if (!st) return -1;
    if (success) {
        st->packets_inserted++;
        if (depth > st->max_depth) st->max_depth = depth;
    } else {
        st->discards++;
    }
    return 0;
}

int reorder_stats_record_deliver(reorder_stats_t *st, int timed_out) {
    if (!st) return -1;
    st->packets_delivered++;
    if (timed_out) st->late_flushes++;
    return 0;
}

int reorder_stats_snapshot(const reorder_stats_t *st, reorder_stats_snapshot_t *out) {
    if (!st || !out) return -1;
    out->packets_inserted  = st->packets_inserted;
    out->packets_delivered = st->packets_delivered;
    out->late_flushes      = st->late_flushes;
    out->discards          = st->discards;
    out->max_depth         = st->max_depth;
    return 0;
}
