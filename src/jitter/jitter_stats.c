/*
 * jitter_stats.c — Jitter statistics implementation (RFC 3550)
 */

#include "jitter_stats.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

struct jitter_stats_s {
    jitter_stats_snapshot_t snap;
    /* For RFC 3550 jitter calculation */
    int64_t  prev_transit;    /* previous transit time difference */
    int      has_prev;
};

jitter_stats_t *jitter_stats_create(void) {
    jitter_stats_t *st = calloc(1, sizeof(*st));
    if (st) st->snap.min_delay_us = DBL_MAX;
    return st;
}

void jitter_stats_destroy(jitter_stats_t *st) {
    free(st);
}

void jitter_stats_reset(jitter_stats_t *st) {
    if (st) {
        memset(&st->snap, 0, sizeof(st->snap));
        st->snap.min_delay_us = DBL_MAX;
        st->prev_transit = 0;
        st->has_prev = 0;
    }
}

int jitter_stats_record_arrival(jitter_stats_t *st,
                                  uint64_t        send_us,
                                  uint64_t        recv_us,
                                  int             is_late,
                                  int             was_dropped) {
    if (!st) return -1;

    st->snap.packets_received++;
    if (is_late)      st->snap.packets_late++;
    if (was_dropped)  st->snap.packets_dropped++;

    /* Delay */
    double delay_us = (recv_us >= send_us) ?
                      (double)(recv_us - send_us) : 0.0;

    /* Update min/max */
    if (delay_us < st->snap.min_delay_us) st->snap.min_delay_us = delay_us;
    if (delay_us > st->snap.max_delay_us) st->snap.max_delay_us = delay_us;

    /* Running mean (Welford) */
    double n = (double)st->snap.packets_received;
    st->snap.avg_delay_us += (delay_us - st->snap.avg_delay_us) / n;

    /* RFC 3550 inter-arrival jitter */
    int64_t transit = (int64_t)(recv_us - send_us);
    if (st->has_prev) {
        int64_t d = transit - st->prev_transit;
        if (d < 0) d = -d;
        st->snap.jitter_us += ((double)d - st->snap.jitter_us) / 16.0;
    }
    st->prev_transit = transit;
    st->has_prev = 1;

    return 0;
}

int jitter_stats_snapshot(const jitter_stats_t    *st,
                            jitter_stats_snapshot_t *out) {
    if (!st || !out) return -1;
    *out = st->snap;
    /* Normalise min to 0 if no packets received */
    if (out->packets_received == 0) out->min_delay_us = 0.0;
    return 0;
}
