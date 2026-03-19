/*
 * loss_stats.c — Packet loss statistics implementation
 */

#include "loss_stats.h"

#include <stdlib.h>
#include <string.h>

struct loss_stats_s {
    uint64_t total_sent;
    uint64_t total_lost;
    uint32_t burst_count;
    uint32_t max_burst;
    uint32_t current_burst; /* ongoing consecutive loss count */
    int last_was_lost;
};

loss_stats_t *loss_stats_create(void) {
    return calloc(1, sizeof(loss_stats_t));
}

void loss_stats_destroy(loss_stats_t *st) {
    free(st);
}

void loss_stats_reset(loss_stats_t *st) {
    if (st)
        memset(st, 0, sizeof(*st));
}

int loss_stats_record(loss_stats_t *st, int lost) {
    if (!st)
        return -1;
    st->total_sent++;
    if (lost) {
        st->total_lost++;
        st->current_burst++;
        if (!st->last_was_lost) {
            st->burst_count++; /* new burst started */
        }
        if (st->current_burst > st->max_burst)
            st->max_burst = st->current_burst;
        st->last_was_lost = 1;
    } else {
        st->current_burst = 0;
        st->last_was_lost = 0;
    }
    return 0;
}

int loss_stats_snapshot(const loss_stats_t *st, loss_stats_snapshot_t *out) {
    if (!st || !out)
        return -1;
    out->total_sent = st->total_sent;
    out->total_lost = st->total_lost;
    out->burst_count = st->burst_count;
    out->max_burst = st->max_burst;
    out->loss_pct =
        (st->total_sent > 0) ? (double)st->total_lost / (double)st->total_sent * 100.0 : 0.0;
    return 0;
}
