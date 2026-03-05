/*
 * dq_queue.c — Drain queue FIFO implementation
 */

#include "dq_queue.h"
#include <stdlib.h>
#include <string.h>

struct dq_queue_s {
    dq_entry_t data[DQ_MAX_ENTRIES];
    int        head;    /* dequeue from head */
    int        tail;    /* enqueue at tail */
    int        count;
    uint64_t   next_seq;
};

dq_queue_t *dq_queue_create(void) { return calloc(1, sizeof(dq_queue_t)); }
void        dq_queue_destroy(dq_queue_t *q) { free(q); }
int         dq_queue_count(const dq_queue_t *q) { return q ? q->count : 0; }
void        dq_queue_clear(dq_queue_t *q) {
    if (q) { q->head = q->tail = q->count = 0; }
}

int dq_queue_enqueue(dq_queue_t *q, const dq_entry_t *e) {
    if (!q || !e || q->count >= DQ_MAX_ENTRIES) return -1;
    q->data[q->tail]      = *e;
    q->data[q->tail].seq  = q->next_seq++;
    q->tail = (q->tail + 1) % DQ_MAX_ENTRIES;
    q->count++;
    return 0;
}

int dq_queue_dequeue(dq_queue_t *q, dq_entry_t *out) {
    if (!q || !out || q->count == 0) return -1;
    *out   = q->data[q->head];
    q->head = (q->head + 1) % DQ_MAX_ENTRIES;
    q->count--;
    return 0;
}

int dq_queue_drain_all(dq_queue_t *q, dq_drain_fn cb, void *user) {
    if (!q) return 0;
    int n = 0;
    dq_entry_t e;
    while (dq_queue_dequeue(q, &e) == 0) {
        if (cb) cb(&e, user);
        n++;
    }
    return n;
}
