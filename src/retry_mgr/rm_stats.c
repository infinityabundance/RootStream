/*
 * rm_stats.c — Retry manager statistics implementation
 */

#include "rm_stats.h"
#include <stdlib.h>
#include <string.h>

struct rm_stats_s {
    uint64_t total_attempts;
    uint32_t total_succeeded;
    uint32_t total_expired;
    uint32_t max_attempts_seen;
};

rm_stats_t *rm_stats_create(void) {
    return calloc(1, sizeof(rm_stats_t));
}

void rm_stats_destroy(rm_stats_t *st) { free(st); }

void rm_stats_reset(rm_stats_t *st) {
    if (st) memset(st, 0, sizeof(*st));
}

int rm_stats_record_attempt(rm_stats_t *st, uint32_t attempt_count) {
    if (!st) return -1;
    st->total_attempts++;
    if (attempt_count > st->max_attempts_seen)
        st->max_attempts_seen = attempt_count;
    return 0;
}

int rm_stats_record_success(rm_stats_t *st) {
    if (!st) return -1;
    st->total_succeeded++;
    return 0;
}

int rm_stats_record_expire(rm_stats_t *st) {
    if (!st) return -1;
    st->total_expired++;
    return 0;
}

int rm_stats_snapshot(const rm_stats_t *st, rm_stats_snapshot_t *out) {
    if (!st || !out) return -1;
    out->total_attempts    = st->total_attempts;
    out->total_succeeded   = st->total_succeeded;
    out->total_expired     = st->total_expired;
    out->max_attempts_seen = st->max_attempts_seen;
    return 0;
}
