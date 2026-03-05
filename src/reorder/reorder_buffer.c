/*
 * reorder_buffer.c — Sequence-ordered packet reorder buffer
 *
 * Internal storage: a flat array of REORDER_BUFFER_CAPACITY slots,
 * keyed by (seq % CAPACITY).  Sequence ordering uses 16-bit serial
 * arithmetic (RFC 1982).
 */

#include "reorder_buffer.h"

#include <stdlib.h>
#include <string.h>

/* RFC 1982 16-bit serial comparison: returns true if s1 < s2 */
static bool seq_lt(uint16_t s1, uint16_t s2) {
    return (int16_t)(s1 - s2) < 0;
}

struct reorder_buffer_s {
    reorder_slot_t     slots[REORDER_BUFFER_CAPACITY];
    uint16_t           next_seq;  /* next expected delivery seq (starts at 0) */
    int                count;
    uint64_t           timeout_us;
    reorder_deliver_fn deliver;
    void              *user;
};

reorder_buffer_t *reorder_buffer_create(uint64_t          timeout_us,
                                          reorder_deliver_fn deliver,
                                          void              *user) {
    if (timeout_us == 0) return NULL;
    reorder_buffer_t *rb = calloc(1, sizeof(*rb));
    if (!rb) return NULL;
    rb->timeout_us = timeout_us;
    rb->deliver    = deliver;
    rb->user       = user;
    return rb;
}

void reorder_buffer_destroy(reorder_buffer_t *rb) { free(rb); }

int reorder_buffer_count(const reorder_buffer_t *rb) { return rb ? rb->count : 0; }

int reorder_buffer_set_timeout(reorder_buffer_t *rb, uint64_t timeout_us) {
    if (!rb || timeout_us == 0) return -1;
    rb->timeout_us = timeout_us;
    return 0;
}

int reorder_buffer_insert(reorder_buffer_t *rb,
                            uint16_t          seq,
                            uint64_t          arrival_us,
                            const uint8_t    *payload,
                            uint16_t          payload_len) {
    if (!rb) return -1;
    if (rb->count >= REORDER_BUFFER_CAPACITY) return -1;

    int idx = seq % REORDER_BUFFER_CAPACITY;
    if (rb->slots[idx].occupied) return -1; /* collision / duplicate */

    int rc = reorder_slot_fill(&rb->slots[idx], seq, arrival_us, payload, payload_len);
    if (rc < 0) return -1;
    rb->count++;
    return 0;
}

int reorder_buffer_flush(reorder_buffer_t *rb, uint64_t now_us) {
    if (!rb) return 0;
    int delivered = 0;

    /* Deliver consecutive in-order packets */
    for (;;) {
        int idx = rb->next_seq % REORDER_BUFFER_CAPACITY;
        reorder_slot_t *slot = &rb->slots[idx];
        if (!slot->occupied || slot->seq != rb->next_seq)
            break;
        if (rb->deliver) rb->deliver(slot, rb->user);
        reorder_slot_clear(slot);
        rb->count--;
        rb->next_seq++;
        delivered++;
    }

    /* Timeout flush: deliver the oldest timed-out packet to unblock */
    for (int i = 0; i < REORDER_BUFFER_CAPACITY && rb->count > 0; i++) {
        reorder_slot_t *slot = &rb->slots[i];
        if (!slot->occupied) continue;
        if (slot->arrival_us + rb->timeout_us <= now_us) {
            /* Advance next_seq to this slot's seq to restore order */
            if (seq_lt(rb->next_seq, slot->seq))
                rb->next_seq = slot->seq;
            if (rb->deliver) rb->deliver(slot, rb->user);
            reorder_slot_clear(slot);
            rb->count--;
            rb->next_seq++;
            delivered++;
            /* Restart in-order delivery pass */
            for (;;) {
                int idx2 = rb->next_seq % REORDER_BUFFER_CAPACITY;
                reorder_slot_t *s2 = &rb->slots[idx2];
                if (!s2->occupied || s2->seq != rb->next_seq) break;
                if (rb->deliver) rb->deliver(s2, rb->user);
                reorder_slot_clear(s2);
                rb->count--;
                rb->next_seq++;
                delivered++;
            }
        }
    }

    return delivered;
}
