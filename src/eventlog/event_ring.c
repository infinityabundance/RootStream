/*
 * event_ring.c — Circular event log ring buffer
 */

#include "event_ring.h"

#include <stdlib.h>
#include <string.h>

struct event_ring_s {
    event_entry_t entries[EVENT_RING_CAPACITY];
    int head;  /* next write slot */
    int count; /* valid entries */
};

event_ring_t *event_ring_create(void) {
    return calloc(1, sizeof(event_ring_t));
}

void event_ring_destroy(event_ring_t *r) {
    free(r);
}

int event_ring_count(const event_ring_t *r) {
    return r ? r->count : 0;
}

bool event_ring_is_empty(const event_ring_t *r) {
    return !r || r->count == 0;
}

void event_ring_clear(event_ring_t *r) {
    if (r) {
        r->head = 0;
        r->count = 0;
    }
}

int event_ring_push(event_ring_t *r, const event_entry_t *e) {
    if (!r || !e)
        return -1;
    r->entries[r->head] = *e;
    r->head = (r->head + 1) % EVENT_RING_CAPACITY;
    if (r->count < EVENT_RING_CAPACITY)
        r->count++;
    return 0;
}

int event_ring_get(const event_ring_t *r, int age, event_entry_t *out) {
    if (!r || !out || age < 0 || age >= r->count)
        return -1;
    /* newest is at (head - 1) going backwards */
    int idx = (r->head - 1 - age + EVENT_RING_CAPACITY * 2) % EVENT_RING_CAPACITY;
    *out = r->entries[idx];
    return 0;
}

int event_ring_find_level(const event_ring_t *r, event_level_t min_level, int *out_ages,
                          int max_results) {
    if (!r || !out_ages || max_results <= 0)
        return 0;
    int found = 0;
    for (int age = 0; age < r->count && found < max_results; age++) {
        event_entry_t e;
        event_ring_get(r, age, &e);
        if (e.level >= min_level)
            out_ages[found++] = age;
    }
    return found;
}
