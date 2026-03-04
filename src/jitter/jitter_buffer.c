/*
 * jitter_buffer.c — Jitter buffer implementation (sorted insertion array)
 */

#include "jitter_buffer.h"

#include <stdlib.h>
#include <string.h>

struct jitter_buffer_s {
    jitter_packet_t  pkts[JITTER_BUF_CAPACITY];
    size_t           count;
    uint64_t         playout_delay_us;
};

jitter_buffer_t *jitter_buffer_create(uint64_t playout_delay_us) {
    jitter_buffer_t *b = calloc(1, sizeof(*b));
    if (!b) return NULL;
    b->playout_delay_us = playout_delay_us;
    return b;
}

void jitter_buffer_destroy(jitter_buffer_t *buf) {
    free(buf);
}

size_t jitter_buffer_count(const jitter_buffer_t *buf) {
    return buf ? buf->count : 0;
}

bool jitter_buffer_is_empty(const jitter_buffer_t *buf) {
    return buf ? (buf->count == 0) : true;
}

void jitter_buffer_flush(jitter_buffer_t *buf) {
    if (buf) buf->count = 0;
}

int jitter_buffer_push(jitter_buffer_t       *buf,
                         const jitter_packet_t *pkt) {
    if (!buf || !pkt) return -1;

    if (buf->count >= JITTER_BUF_CAPACITY) {
        /* Drop the oldest (first) packet to make room */
        memmove(&buf->pkts[0], &buf->pkts[1],
                (JITTER_BUF_CAPACITY - 1) * sizeof(jitter_packet_t));
        buf->count--;
    }

    /* Sorted insertion by seq_num (ascending) */
    size_t ins = buf->count;
    for (size_t i = 0; i < buf->count; i++) {
        if (jitter_packet_before(pkt->seq_num, buf->pkts[i].seq_num)) {
            ins = i;
            break;
        }
    }

    if (ins < buf->count) {
        memmove(&buf->pkts[ins + 1], &buf->pkts[ins],
                (buf->count - ins) * sizeof(jitter_packet_t));
    }
    buf->pkts[ins] = *pkt;
    buf->count++;
    return 0;
}

int jitter_buffer_peek(const jitter_buffer_t *buf, jitter_packet_t *out) {
    if (!buf || !out || buf->count == 0) return -1;
    *out = buf->pkts[0];
    return 0;
}

int jitter_buffer_pop(jitter_buffer_t *buf,
                        uint64_t         now_us,
                        jitter_packet_t *out) {
    if (!buf || !out || buf->count == 0) return -1;

    jitter_packet_t *oldest = &buf->pkts[0];
    uint64_t playout_time   = oldest->capture_us + buf->playout_delay_us;

    if (now_us < playout_time) return -1; /* Not due yet */

    *out = *oldest;
    /* Mark as late if we're significantly past the deadline */
    if (now_us > playout_time + buf->playout_delay_us)
        out->flags |= JITTER_FLAG_LATE;

    memmove(&buf->pkts[0], &buf->pkts[1],
            (buf->count - 1) * sizeof(jitter_packet_t));
    buf->count--;
    return 0;
}
