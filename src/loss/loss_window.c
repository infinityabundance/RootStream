/*
 * loss_window.c — Sliding packet loss window
 */

#include "loss_window.h"

#include <string.h>

int lw_init(loss_window_t *w) {
    if (!w) return -1;
    memset(w, 0, sizeof(*w));
    return 0;
}

void lw_reset(loss_window_t *w) {
    if (w) memset(w, 0, sizeof(*w));
}

/* Advance window to accommodate seq, marking skipped slots lost */
static void lw_advance(loss_window_t *w, uint16_t seq) {
    /* How many slots to advance? */
    int delta = (int)(uint16_t)(seq - w->base_seq);
    if (delta <= 0) return; /* old or duplicate */

    if (delta >= LOSS_WIN_SIZE) {
        /* All slots are now stale — mark the whole window lost */
        int lost_count = LOSS_WIN_SIZE - __builtin_popcountll(w->received_mask);
        w->total_lost += (uint32_t)lost_count;
        w->total_seen += (uint32_t)LOSS_WIN_SIZE;
        w->received_mask = 0;
        w->base_seq = seq;
        return;
    }

    /* Slide forward by delta slots */
    for (int i = 0; i < delta; i++) {
        int slot = (w->base_seq + i) % LOSS_WIN_SIZE;
        uint64_t bit = (uint64_t)1 << slot;
        if (!(w->received_mask & bit)) {
            /* This slot was not received → count as lost */
            w->total_lost++;
        }
        w->total_seen++;
        w->received_mask &= ~bit; /* clear for reuse */
    }
    w->base_seq = (uint16_t)(w->base_seq + delta);
}

int lw_receive(loss_window_t *w, uint16_t seq) {
    if (!w) return -1;

    if (!w->initialised) {
        w->base_seq    = seq;
        w->initialised = true;
    }

    /* Advance window if needed */
    int16_t delta = (int16_t)(seq - w->base_seq);
    if (delta > 0) lw_advance(w, seq);

    /* Mark slot received */
    int slot = seq % LOSS_WIN_SIZE;
    uint64_t bit = (uint64_t)1 << slot;
    w->received_mask |= bit;
    return 0;
}

double lw_loss_rate(const loss_window_t *w) {
    if (!w || w->total_seen == 0) return 0.0;
    return (double)w->total_lost / (double)w->total_seen;
}
