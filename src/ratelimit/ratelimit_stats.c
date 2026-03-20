/*
 * ratelimit_stats.c — Rate limiter statistics implementation
 */

#include "ratelimit_stats.h"

#include <stdlib.h>
#include <string.h>

struct ratelimit_stats_s {
    uint64_t packets_allowed;
    uint64_t packets_throttled;
    double bytes_consumed;
};

ratelimit_stats_t *ratelimit_stats_create(void) {
    return calloc(1, sizeof(ratelimit_stats_t));
}

void ratelimit_stats_destroy(ratelimit_stats_t *st) {
    free(st);
}

void ratelimit_stats_reset(ratelimit_stats_t *st) {
    if (st)
        memset(st, 0, sizeof(*st));
}

int ratelimit_stats_record(ratelimit_stats_t *st, int allowed, double bytes) {
    if (!st)
        return -1;
    if (allowed) {
        st->packets_allowed++;
        st->bytes_consumed += bytes;
    } else {
        st->packets_throttled++;
    }
    return 0;
}

int ratelimit_stats_snapshot(const ratelimit_stats_t *st, ratelimit_stats_snapshot_t *out) {
    if (!st || !out)
        return -1;
    out->packets_allowed = st->packets_allowed;
    out->packets_throttled = st->packets_throttled;
    out->bytes_consumed = st->bytes_consumed;
    uint64_t total = st->packets_allowed + st->packets_throttled;
    out->throttle_rate = (total > 0) ? (double)st->packets_throttled / (double)total : 0.0;
    return 0;
}
