/*
 * plc_stats.c — PLC statistics implementation
 *
 * Loss rate is computed over a sliding window of PLC_STATS_WINDOW events
 * using a circular bitset: each slot is 1 (lost) or 0 (received).
 */

#include "plc_stats.h"

#include <stdlib.h>
#include <string.h>

struct plc_stats_s {
    uint64_t frames_received;
    uint64_t frames_lost;
    uint64_t concealment_events;
    int      max_consecutive_loss;
    int      current_run;          /* current consecutive loss run */

    /* Sliding window for loss rate */
    uint8_t  window[PLC_STATS_WINDOW]; /* 0=received, 1=lost */
    int      win_head;
    int      win_count;
    int      win_lost_count;
};

plc_stats_t *plc_stats_create(void) {
    return calloc(1, sizeof(plc_stats_t));
}

void plc_stats_destroy(plc_stats_t *st) {
    free(st);
}

void plc_stats_reset(plc_stats_t *st) {
    if (st) memset(st, 0, sizeof(*st));
}

static void window_push(plc_stats_t *st, int is_lost) {
    if (st->win_count >= PLC_STATS_WINDOW) {
        /* Evict oldest */
        int oldest = (st->win_head - st->win_count + PLC_STATS_WINDOW * 2)
                     % PLC_STATS_WINDOW;
        if (st->window[oldest]) st->win_lost_count--;
        st->win_count--;
    }
    int slot = st->win_head;
    st->window[slot] = (uint8_t)is_lost;
    if (is_lost) st->win_lost_count++;
    st->win_head = (st->win_head + 1) % PLC_STATS_WINDOW;
    st->win_count++;
}

int plc_stats_record_received(plc_stats_t *st) {
    if (!st) return -1;
    st->frames_received++;
    st->current_run = 0;
    window_push(st, 0);
    return 0;
}

int plc_stats_record_lost(plc_stats_t *st, int is_new_burst) {
    if (!st) return -1;
    st->frames_lost++;
    if (is_new_burst) st->concealment_events++;
    st->current_run++;
    if (st->current_run > st->max_consecutive_loss)
        st->max_consecutive_loss = st->current_run;
    window_push(st, 1);
    return 0;
}

int plc_stats_snapshot(const plc_stats_t *st, plc_stats_snapshot_t *out) {
    if (!st || !out) return -1;
    out->frames_received      = st->frames_received;
    out->frames_lost          = st->frames_lost;
    out->concealment_events   = st->concealment_events;
    out->max_consecutive_loss = st->max_consecutive_loss;
    out->loss_rate = (st->win_count > 0) ?
                     (double)st->win_lost_count / (double)st->win_count : 0.0;
    return 0;
}
