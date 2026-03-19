/*
 * eb_stats.c — Event bus statistics implementation
 */

#include "eb_stats.h"

#include <stdlib.h>
#include <string.h>

struct eb_stats_s {
    uint64_t published_count;
    uint64_t dispatch_count;
    uint64_t dropped_count;
};

eb_stats_t *eb_stats_create(void) {
    return calloc(1, sizeof(eb_stats_t));
}

void eb_stats_destroy(eb_stats_t *st) {
    free(st);
}

void eb_stats_reset(eb_stats_t *st) {
    if (st)
        memset(st, 0, sizeof(*st));
}

int eb_stats_record_publish(eb_stats_t *st, int dispatch_n) {
    if (!st)
        return -1;
    st->published_count++;
    if (dispatch_n <= 0) {
        st->dropped_count++;
    } else {
        st->dispatch_count += (uint64_t)dispatch_n;
    }
    return 0;
}

int eb_stats_snapshot(const eb_stats_t *st, eb_stats_snapshot_t *out) {
    if (!st || !out)
        return -1;
    out->published_count = st->published_count;
    out->dispatch_count = st->dispatch_count;
    out->dropped_count = st->dropped_count;
    return 0;
}
