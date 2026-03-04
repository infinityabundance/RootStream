/*
 * event_ring.c — Analytics event ring buffer implementation
 */

#include "event_ring.h"

#include <stdlib.h>
#include <string.h>

struct event_ring_s {
    analytics_event_t buf[EVENT_RING_CAPACITY];
    size_t            head;   /* index of oldest event */
    size_t            tail;   /* index of next write slot */
    size_t            count;
};

event_ring_t *event_ring_create(void) {
    event_ring_t *r = calloc(1, sizeof(*r));
    return r;
}

void event_ring_destroy(event_ring_t *r) {
    free(r);
}

void event_ring_clear(event_ring_t *r) {
    if (!r) return;
    r->head  = 0;
    r->tail  = 0;
    r->count = 0;
}

size_t event_ring_count(const event_ring_t *r) {
    return r ? r->count : 0;
}

bool event_ring_is_empty(const event_ring_t *r) {
    return r ? (r->count == 0) : true;
}

int event_ring_push(event_ring_t            *r,
                     const analytics_event_t *event) {
    if (!r || !event) return -1;

    if (r->count == EVENT_RING_CAPACITY) {
        /* Overwrite oldest — advance head */
        r->head = (r->head + 1) % EVENT_RING_CAPACITY;
        r->count--;
    }

    r->buf[r->tail] = *event;
    r->tail = (r->tail + 1) % EVENT_RING_CAPACITY;
    r->count++;
    return 0;
}

int event_ring_pop(event_ring_t *r, analytics_event_t *out) {
    if (!r || !out || r->count == 0) return -1;
    *out   = r->buf[r->head];
    r->head = (r->head + 1) % EVENT_RING_CAPACITY;
    r->count--;
    return 0;
}

int event_ring_peek(const event_ring_t *r, analytics_event_t *out) {
    if (!r || !out || r->count == 0) return -1;
    *out = r->buf[r->head];
    return 0;
}

size_t event_ring_drain(event_ring_t      *r,
                         analytics_event_t *out,
                         size_t             max) {
    if (!r || !out || max == 0) return 0;
    size_t n = 0;
    while (n < max && r->count > 0) {
        out[n++] = r->buf[r->head];
        r->head = (r->head + 1) % EVENT_RING_CAPACITY;
        r->count--;
    }
    return n;
}
