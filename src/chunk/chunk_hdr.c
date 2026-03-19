/*
 * chunk_hdr.c — Chunk header implementation
 */

#include "chunk_hdr.h"

#include <string.h>

int chunk_hdr_init(chunk_hdr_t *h, uint32_t stream_id, uint32_t frame_seq, uint16_t chunk_idx,
                   uint16_t chunk_count, uint16_t data_len, uint8_t flags) {
    if (!h || chunk_count == 0 || chunk_idx >= chunk_count)
        return -1;
    memset(h, 0, sizeof(*h));
    h->stream_id = stream_id;
    h->frame_seq = frame_seq;
    h->chunk_idx = chunk_idx;
    h->chunk_count = chunk_count;
    h->data_len = data_len;
    h->flags = flags;
    return 0;
}
