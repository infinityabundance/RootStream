/*
 * chunk_split.h — Chunk Splitter: frame → MTU-sized chunks
 *
 * Splits an encoded frame buffer into an array of (header, data-slice)
 * pairs, each with at most `mtu` payload bytes.  The caller provides a
 * pre-allocated output array; the function fills it in and returns the
 * number of chunks produced.
 *
 * No heap allocation is performed — the output slices point directly
 * into the caller-supplied `frame_data` buffer.
 *
 * Thread-safety: stateless — re-entrant.
 */

#ifndef ROOTSTREAM_CHUNK_SPLIT_H
#define ROOTSTREAM_CHUNK_SPLIT_H

#include <stddef.h>
#include <stdint.h>

#include "chunk_hdr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CHUNK_SPLIT_MAX 256 /**< Maximum chunks per frame */

/** One output chunk: header + pointer into source buffer */
typedef struct {
    chunk_hdr_t hdr;  /**< Filled-in chunk header */
    const void *data; /**< Pointer into caller's frame_data */
} chunk_t;

/**
 * chunk_split — split a frame into chunks
 *
 * @param frame_data  Source frame buffer
 * @param frame_len   Source frame length (bytes)
 * @param mtu         Maximum payload bytes per chunk (> 0)
 * @param stream_id   Stream identifier (written to each header)
 * @param frame_seq   Frame sequence number (written to each header)
 * @param flags       Base CHUNK_FLAG_* bits (CHUNK_FLAG_LAST set on last)
 * @param out         Output array of chunk_t (caller-allocated)
 * @param max_out     Size of out array
 * @return            Number of chunks produced, or -1 on invalid params
 */
int chunk_split(const void *frame_data, size_t frame_len, size_t mtu, uint32_t stream_id,
                uint32_t frame_seq, uint8_t flags, chunk_t *out, int max_out);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CHUNK_SPLIT_H */
