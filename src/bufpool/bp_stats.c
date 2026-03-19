/*
 * bp_stats.c — Buffer pool statistics implementation
 */

#include "bp_stats.h"

#include <stdlib.h>
#include <string.h>

struct bp_stats_s {
    uint64_t alloc_count;
    uint64_t free_count;
    int peak_in_use;
    uint64_t fail_count;
};

bp_stats_t *bp_stats_create(void) {
    return calloc(1, sizeof(bp_stats_t));
}

void bp_stats_destroy(bp_stats_t *st) {
    free(st);
}

void bp_stats_reset(bp_stats_t *st) {
    if (st)
        memset(st, 0, sizeof(*st));
}

int bp_stats_record_alloc(bp_stats_t *st, int in_use) {
    if (!st)
        return -1;
    st->alloc_count++;
    if (in_use > st->peak_in_use)
        st->peak_in_use = in_use;
    return 0;
}

int bp_stats_record_free(bp_stats_t *st) {
    if (!st)
        return -1;
    st->free_count++;
    return 0;
}

int bp_stats_record_fail(bp_stats_t *st) {
    if (!st)
        return -1;
    st->fail_count++;
    return 0;
}

int bp_stats_snapshot(const bp_stats_t *st, bp_stats_snapshot_t *out) {
    if (!st || !out)
        return -1;
    out->alloc_count = st->alloc_count;
    out->free_count = st->free_count;
    out->peak_in_use = st->peak_in_use;
    out->fail_count = st->fail_count;
    return 0;
}
