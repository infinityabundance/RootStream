/*
 * plc_history.c — Ring buffer of recent good PLC frames
 */

#include "plc_history.h"

#include <stdlib.h>
#include <string.h>

struct plc_history_s {
    plc_frame_t frames[PLC_HISTORY_DEPTH];
    int head;  /* index of the next slot to write */
    int count; /* number of valid entries (capped at DEPTH) */
};

plc_history_t *plc_history_create(void) {
    return calloc(1, sizeof(plc_history_t));
}

void plc_history_destroy(plc_history_t *h) {
    free(h);
}

int plc_history_count(const plc_history_t *h) {
    return h ? h->count : 0;
}

bool plc_history_is_empty(const plc_history_t *h) {
    return !h || h->count == 0;
}

void plc_history_clear(plc_history_t *h) {
    if (h) {
        h->head = 0;
        h->count = 0;
    }
}

int plc_history_push(plc_history_t *h, const plc_frame_t *frame) {
    if (!h || !frame)
        return -1;
    h->frames[h->head] = *frame;
    h->head = (h->head + 1) % PLC_HISTORY_DEPTH;
    if (h->count < PLC_HISTORY_DEPTH)
        h->count++;
    return 0;
}

int plc_history_get_last(const plc_history_t *h, plc_frame_t *out) {
    return plc_history_get(h, 0, out);
}

int plc_history_get(const plc_history_t *h, int age, plc_frame_t *out) {
    if (!h || !out || age < 0 || age >= h->count)
        return -1;
    /* newest is at (head - 1), going backwards */
    int idx = (h->head - 1 - age + PLC_HISTORY_DEPTH * 2) % PLC_HISTORY_DEPTH;
    *out = h->frames[idx];
    return 0;
}
