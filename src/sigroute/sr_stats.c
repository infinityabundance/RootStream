/*
 * sr_stats.c — Signal router statistics
 */

#include "sr_stats.h"
#include <stdlib.h>
#include <string.h>

struct sr_stats_s {
    uint64_t routed;
    uint64_t filtered;
    uint64_t dropped;
};

sr_stats_t *sr_stats_create(void) { return calloc(1, sizeof(sr_stats_t)); }
void        sr_stats_destroy(sr_stats_t *st) { free(st); }
void        sr_stats_reset(sr_stats_t *st) { if (st) memset(st, 0, sizeof(*st)); }

int sr_stats_record_route(sr_stats_t *st, int delivered, int filtered_n) {
    if (!st) return -1;
    if (delivered == 0 && filtered_n == 0) {
        st->dropped++;
    } else {
        if (delivered > 0)  st->routed++;
        if (filtered_n > 0) st->filtered += (uint64_t)filtered_n;
    }
    return 0;
}

int sr_stats_snapshot(const sr_stats_t *st, sr_stats_snapshot_t *out) {
    if (!st || !out) return -1;
    out->routed   = st->routed;
    out->filtered = st->filtered;
    out->dropped  = st->dropped;
    return 0;
}
