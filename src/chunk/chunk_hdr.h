/*
 * chunk_hdr.h — Chunk Splitter: chunk header descriptor
 *
 * A chunk header identifies one network-transmittable piece of an
 * encoded frame.  The combination (stream_id, frame_seq, chunk_idx)
 * uniquely identifies a chunk within a stream.
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_CHUNK_HDR_H
#define ROOTSTREAM_CHUNK_HDR_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Chunk flags */
#define CHUNK_FLAG_KEYFRAME  0x01u  /**< Frame is a keyframe */
#define CHUNK_FLAG_LAST      0x02u  /**< This is the last chunk of the frame */

/** Chunk header (fits in 16 bytes on-wire) */
typedef struct {
    uint32_t stream_id;     /**< Source stream identifier */
    uint32_t frame_seq;     /**< Frame sequence number (wraps) */
    uint16_t chunk_idx;     /**< Zero-based chunk index within frame */
    uint16_t chunk_count;   /**< Total chunks for this frame (≥ 1) */
    uint16_t data_len;      /**< Payload byte length for this chunk */
    uint8_t  flags;         /**< CHUNK_FLAG_* bitmask */
    uint8_t  _pad;          /**< Reserved */
} chunk_hdr_t;

/**
 * chunk_hdr_init — initialise a chunk header
 *
 * @return 0 on success, -1 on NULL or invalid params
 */
int chunk_hdr_init(chunk_hdr_t *h,
                   uint32_t     stream_id,
                   uint32_t     frame_seq,
                   uint16_t     chunk_idx,
                   uint16_t     chunk_count,
                   uint16_t     data_len,
                   uint8_t      flags);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CHUNK_HDR_H */
