/*
 * sl_stats.c — Session limiter statistics implementation
 */

#include "sl_stats.h"
#include <stdlib.h>
#include <string.h>

struct sl_stats_s {
    uint32_t total_admitted;
    uint32_t total_rejected;
    uint32_t peak_count;
    uint32_t eviction_count;
};

sl_stats_t *sl_stats_create(void) {
    return calloc(1, sizeof(sl_stats_t));
}

void sl_stats_destroy(sl_stats_t *st) { free(st); }

void sl_stats_reset(sl_stats_t *st) {
    if (st) memset(st, 0, sizeof(*st));
}

int sl_stats_record_admit(sl_stats_t *st, int current_count) {
    if (!st) return -1;
    st->total_admitted++;
    if ((uint32_t)current_count > st->peak_count)
        st->peak_count = (uint32_t)current_count;
    return 0;
}

int sl_stats_record_reject(sl_stats_t *st) {
    if (!st) return -1;
    st->total_rejected++;
    return 0;
}

int sl_stats_record_eviction(sl_stats_t *st) {
    if (!st) return -1;
    st->eviction_count++;
    return 0;
}

int sl_stats_snapshot(const sl_stats_t *st, sl_stats_snapshot_t *out) {
    if (!st || !out) return -1;
    out->total_admitted = st->total_admitted;
    out->total_rejected = st->total_rejected;
    out->peak_count     = st->peak_count;
    out->eviction_count = st->eviction_count;
    return 0;
}
