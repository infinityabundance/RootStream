/*
 * ts_stats.c — Timestamp statistics implementation
 */

#include "ts_stats.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct ts_stats_s {
    uint64_t sample_count;
    int64_t max_drift_us;
    int64_t total_correction_us;
};

ts_stats_t *ts_stats_create(void) {
    return calloc(1, sizeof(ts_stats_t));
}

void ts_stats_destroy(ts_stats_t *st) {
    free(st);
}

void ts_stats_reset(ts_stats_t *st) {
    if (st)
        memset(st, 0, sizeof(*st));
}

int ts_stats_record(ts_stats_t *st, int64_t error_us) {
    if (!st)
        return -1;
    int64_t abs_err = error_us < 0 ? -error_us : error_us;
    st->sample_count++;
    if (abs_err > st->max_drift_us)
        st->max_drift_us = abs_err;
    st->total_correction_us += abs_err;
    return 0;
}

int ts_stats_snapshot(const ts_stats_t *st, ts_stats_snapshot_t *out) {
    if (!st || !out)
        return -1;
    out->sample_count = st->sample_count;
    out->max_drift_us = st->max_drift_us;
    out->total_correction_us = st->total_correction_us;
    return 0;
}
