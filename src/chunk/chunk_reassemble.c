/*
 * chunk_reassemble.c — Chunk reassembly implementation
 */

#include "chunk_reassemble.h"

#include <stdlib.h>
#include <string.h>

struct reassemble_ctx_s {
    reassemble_slot_t slots[REASSEMBLE_SLOTS];
};

reassemble_ctx_t *reassemble_ctx_create(void) {
    return calloc(1, sizeof(reassemble_ctx_t));
}

void reassemble_ctx_destroy(reassemble_ctx_t *c) {
    free(c);
}

int reassemble_count(const reassemble_ctx_t *c) {
    if (!c)
        return 0;
    int n = 0;
    for (int i = 0; i < REASSEMBLE_SLOTS; i++)
        if (c->slots[i].in_use)
            n++;
    return n;
}

static reassemble_slot_t *find_slot(reassemble_ctx_t *c, uint32_t stream_id, uint32_t frame_seq) {
    for (int i = 0; i < REASSEMBLE_SLOTS; i++)
        if (c->slots[i].in_use && c->slots[i].stream_id == stream_id &&
            c->slots[i].frame_seq == frame_seq)
            return &c->slots[i];
    return NULL;
}

reassemble_slot_t *reassemble_receive(reassemble_ctx_t *c, const chunk_hdr_t *h) {
    if (!c || !h)
        return NULL;
    if (h->chunk_count == 0 || h->chunk_count > REASSEMBLE_MAX_CHUNKS ||
        h->chunk_idx >= h->chunk_count)
        return NULL;

    reassemble_slot_t *s = find_slot(c, h->stream_id, h->frame_seq);
    if (!s) {
        /* Open a new slot */
        for (int i = 0; i < REASSEMBLE_SLOTS; i++) {
            if (!c->slots[i].in_use) {
                s = &c->slots[i];
                memset(s, 0, sizeof(*s));
                s->stream_id = h->stream_id;
                s->frame_seq = h->frame_seq;
                s->chunk_count = h->chunk_count;
                s->in_use = true;
                break;
            }
        }
        if (!s)
            return NULL; /* No free slots */
    }

    s->received_mask |= (1u << h->chunk_idx);

    /* Complete when all bits set */
    uint32_t full_mask = (h->chunk_count == 32) ? 0xFFFFFFFFu : (1u << h->chunk_count) - 1u;
    s->complete = (s->received_mask == full_mask);
    return s;
}

int reassemble_release(reassemble_ctx_t *c, reassemble_slot_t *s) {
    if (!c || !s)
        return -1;
    for (int i = 0; i < REASSEMBLE_SLOTS; i++) {
        if (&c->slots[i] == s) {
            memset(s, 0, sizeof(*s));
            return 0;
        }
    }
    return -1;
}
