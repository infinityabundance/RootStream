/*
 * kfr_stats.c — Keyframe request statistics implementation
 */

#include "kfr_stats.h"

#include <stdlib.h>
#include <string.h>

struct kfr_stats_s {
    uint64_t requests_received;
    uint64_t requests_forwarded;
    uint64_t requests_suppressed;
    uint64_t urgent_requests;
};

kfr_stats_t *kfr_stats_create(void) {
    return calloc(1, sizeof(kfr_stats_t));
}

void kfr_stats_destroy(kfr_stats_t *st) {
    free(st);
}

void kfr_stats_reset(kfr_stats_t *st) {
    if (st)
        memset(st, 0, sizeof(*st));
}

int kfr_stats_record(kfr_stats_t *st, int forwarded, int urgent) {
    if (!st)
        return -1;
    st->requests_received++;
    if (forwarded)
        st->requests_forwarded++;
    else
        st->requests_suppressed++;
    if (urgent)
        st->urgent_requests++;
    return 0;
}

int kfr_stats_snapshot(const kfr_stats_t *st, kfr_stats_snapshot_t *out) {
    if (!st || !out)
        return -1;
    out->requests_received = st->requests_received;
    out->requests_forwarded = st->requests_forwarded;
    out->requests_suppressed = st->requests_suppressed;
    out->urgent_requests = st->urgent_requests;
    out->suppression_rate = (st->requests_received > 0)
                                ? (double)st->requests_suppressed / (double)st->requests_received
                                : 0.0;
    return 0;
}
