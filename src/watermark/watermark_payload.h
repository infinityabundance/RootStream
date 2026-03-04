/*
 * watermark_payload.h — Per-viewer watermark payload format
 *
 * A watermark payload is a small binary record that encodes a
 * viewer-specific identifier so that, if a video leak is detected,
 * the original recipient can be traced.
 *
 * Wire encoding (little-endian)
 * ─────────────────────────────
 *  Offset  Size  Field
 *   0      4     Magic      0x574D4B50 ('WMKP')
 *   4      8     viewer_id  — unique 64-bit viewer identifier
 *  12      8     session_id — stream session identifier
 *  20      8     timestamp_us — embedding time (µs epoch)
 *  28      2     payload_bits — number of bits actually embedded
 *  30      2     reserved
 *  32      N     data       — up to WATERMARK_MAX_DATA_BYTES payload bytes
 *
 * The payload is serialised to a bit array before embedding so that
 * both LSB-spatial and DCT-domain embedders share the same bit stream.
 */

#ifndef ROOTSTREAM_WATERMARK_PAYLOAD_H
#define ROOTSTREAM_WATERMARK_PAYLOAD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WATERMARK_MAGIC          0x574D4B50UL  /* 'WMKP' */
#define WATERMARK_HDR_SIZE       32
#define WATERMARK_MAX_DATA_BYTES 8   /* 64 bits — enough for viewer + session */

/** Watermark payload */
typedef struct {
    uint64_t viewer_id;
    uint64_t session_id;
    uint64_t timestamp_us;
    uint16_t payload_bits;   /**< Number of meaningful bits in @data */
    uint8_t  data[WATERMARK_MAX_DATA_BYTES];
} watermark_payload_t;

/**
 * watermark_payload_encode — serialise @payload into @buf
 *
 * @param payload  Payload to encode
 * @param buf      Output buffer (>= WATERMARK_HDR_SIZE + MAX_DATA bytes)
 * @param buf_sz   Size of @buf
 * @return         Bytes written, or -1 on error
 */
int watermark_payload_encode(const watermark_payload_t *payload,
                              uint8_t                   *buf,
                              size_t                     buf_sz);

/**
 * watermark_payload_decode — parse @payload from @buf
 *
 * @param buf      Input buffer
 * @param buf_sz   Valid bytes in @buf
 * @param payload  Output payload
 * @return         0 on success, -1 on error
 */
int watermark_payload_decode(const uint8_t      *buf,
                              size_t              buf_sz,
                              watermark_payload_t *payload);

/**
 * watermark_payload_to_bits — convert payload to a bit array
 *
 * Serialises viewer_id (64 bits) into @bits array; each element is 0 or 1.
 *
 * @param payload   Payload
 * @param bits      Output bit array (must hold >= 64 elements)
 * @param max_bits  Capacity of @bits
 * @return          Number of bits written, or -1 on error
 */
int watermark_payload_to_bits(const watermark_payload_t *payload,
                               uint8_t                   *bits,
                               size_t                     max_bits);

/**
 * watermark_payload_from_bits — reconstruct viewer_id from bit array
 *
 * @param bits     Bit array (each element 0 or 1)
 * @param n_bits   Number of bits (must be 64)
 * @param payload  Output payload (only viewer_id is populated)
 * @return         0 on success, -1 on error
 */
int watermark_payload_from_bits(const uint8_t       *bits,
                                 int                  n_bits,
                                 watermark_payload_t  *payload);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_WATERMARK_PAYLOAD_H */
