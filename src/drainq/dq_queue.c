/*
 * dq_queue.c — Drain queue FIFO implementation
 *
 * DESIGN RATIONALE
 * ----------------
 * A drain queue decouples the encoder thread (producer) from the network
 * sender thread (consumer).  The encoder enqueues completed frames; the
 * sender drains them when the network path is ready.
 *
 * Why a circular FIFO and not a linked list?
 *   Linked lists require one allocation per entry.  Allocating per-frame
 *   on the hot encoding path creates latency spikes and heap fragmentation
 *   at high frame rates (e.g., 120 fps × 16 bytes per node = 1920 allocs/s).
 *   A fixed circular array eliminates per-entry allocation entirely at the
 *   cost of a fixed capacity (DQ_MAX_ENTRIES = 128).  At 120 fps that is
 *   over 1 second of buffer — sufficient for any realistic burst.
 *
 * Sequence numbers:
 *   Assigned by the queue on enqueue (not by the caller).  This guarantees
 *   strict monotonicity: the downstream consumer can detect gaps caused by
 *   dq_queue_clear() or capacity drops by checking seq continuity.
 *
 * clear() vs drain_all(NULL):
 *   dq_queue_clear() is O(1) (just resets indices).  drain_all(NULL, …)
 *   is O(count) but invokes the callback for each entry.  Use clear() when
 *   dropping frames is intentional (e.g., resolution switch).  Use
 *   drain_all() when each entry needs a cleanup step (e.g., freeing payload).
 *
 * Thread-safety: NOT thread-safe.  See dq_queue.h.
 */

#include "dq_queue.h"

#include <stdlib.h>
#include <string.h>

/* ── internal struct ──────────────────────────────────────────────── */

struct dq_queue_s {
    dq_entry_t data[DQ_MAX_ENTRIES]; /* circular buffer of entries        */
    int head;                        /* index of next entry to dequeue    */
    int tail;                        /* index of next free slot (enqueue) */
    int count;                       /* current number of queued entries  */
    uint64_t next_seq;               /* sequence counter; never reset     */
};

/* ── lifecycle ────────────────────────────────────────────────────── */

dq_queue_t *dq_queue_create(void) {
    /* calloc zero-initialises: head=0, tail=0, count=0, next_seq=0. */
    return calloc(1, sizeof(dq_queue_t));
}

void dq_queue_destroy(dq_queue_t *q) {
    /* Does NOT free entry payload pointers — the caller owns payloads.
     * See dq_entry.h for ownership documentation. */
    free(q);
}

int dq_queue_count(const dq_queue_t *q) {
    return q ? q->count : 0;
}

void dq_queue_clear(dq_queue_t *q) {
    /* O(1) discard: just reset the circular buffer indices and count.
     * Sequence counter (next_seq) is intentionally NOT reset — a gap
     * in sequence numbers after clear() is detectable by downstream. */
    if (q) {
        q->head = q->tail = q->count = 0;
    }
}

/* ── enqueue ──────────────────────────────────────────────────────── */

int dq_queue_enqueue(dq_queue_t *q, const dq_entry_t *e) {
    if (!q || !e || q->count >= DQ_MAX_ENTRIES)
        return -1;

    /* Copy the caller's entry into the queue slot.
     * The seq field from *e is ignored and overwritten with next_seq.
     * This ensures the queue — not the caller — controls sequence numbering. */
    q->data[q->tail] = *e;
    q->data[q->tail].seq = q->next_seq++;

    /* Advance tail with modular arithmetic (circular wrap). */
    q->tail = (q->tail + 1) % DQ_MAX_ENTRIES;
    q->count++;
    return 0;
}

/* ── dequeue ──────────────────────────────────────────────────────── */

int dq_queue_dequeue(dq_queue_t *q, dq_entry_t *out) {
    if (!q || !out || q->count == 0)
        return -1;

    /* Copy the head entry to *out, then advance head. */
    *out = q->data[q->head];
    q->head = (q->head + 1) % DQ_MAX_ENTRIES;
    q->count--;
    return 0;
}

/* ── drain_all ────────────────────────────────────────────────────── */

int dq_queue_drain_all(dq_queue_t *q, dq_drain_fn cb, void *user) {
    if (!q)
        return 0;

    int n = 0;
    dq_entry_t e;

    /* Drain in FIFO order.  cb may be NULL (caller only cares about
     * the return count, e.g., for statistics).  The loop uses
     * dq_queue_dequeue() rather than direct index arithmetic so that
     * the head/tail/count invariants are maintained correctly even if
     * cb re-enqueues entries (unusual but not forbidden). */
    while (dq_queue_dequeue(q, &e) == 0) {
        if (cb)
            cb(&e, user);
        n++;
    }
    return n;
}
