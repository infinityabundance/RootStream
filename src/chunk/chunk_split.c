/*
 * chunk_split.c — Frame → chunk splitter
 */

#include "chunk_split.h"

#include <string.h>

int chunk_split(const void *frame_data, size_t frame_len, size_t mtu, uint32_t stream_id,
                uint32_t frame_seq, uint8_t flags, chunk_t *out, int max_out) {
    if (!frame_data || !out || mtu == 0 || max_out <= 0)
        return -1;

    /* Compute total chunks needed */
    int total = (int)((frame_len + mtu - 1) / mtu);
    if (frame_len == 0)
        total = 1; /* empty frame: one zero-length chunk */
    if (total > max_out || total > CHUNK_SPLIT_MAX)
        return -1;

    const uint8_t *src = (const uint8_t *)frame_data;
    size_t offset = 0;

    for (int i = 0; i < total; i++) {
        size_t this_len = (frame_len - offset < mtu) ? (frame_len - offset) : mtu;
        uint8_t f = flags;
        if (i == total - 1)
            f |= CHUNK_FLAG_LAST;

        chunk_hdr_init(&out[i].hdr, stream_id, frame_seq, (uint16_t)i, (uint16_t)total,
                       (uint16_t)this_len, f);
        out[i].data = src + offset;
        offset += this_len;
    }
    return total;
}
