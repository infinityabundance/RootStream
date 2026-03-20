/*
 * watermark_lsb.h — Spatial LSB watermark embed/extract
 *
 * Embeds a bit stream into the least-significant bit of selected luma
 * samples in an 8-bit planar frame.  Uses a simple pseudo-random
 * index sequence seeded from the viewer_id to scatter bits across the
 * frame, providing robustness against simple row/column attacks.
 *
 * Spatial strength means 1 LSB per selected pixel → visually invisible
 * (PSNR drop < 0.01 dB for typical natural images).
 *
 * Thread-safety: embed and extract are stateless and thread-safe.
 */

#ifndef ROOTSTREAM_WATERMARK_LSB_H
#define ROOTSTREAM_WATERMARK_LSB_H

#include <stddef.h>
#include <stdint.h>

#include "watermark_payload.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * watermark_lsb_embed — embed bit stream into 8-bit luma plane (in-place)
 *
 * @param luma       8-bit planar luma buffer (in-place modification)
 * @param width      Frame width in pixels
 * @param height     Frame height in pixels
 * @param stride     Bytes per row (>= width)
 * @param payload    Watermark payload to embed
 * @return           Number of bits embedded, or -1 on error
 */
int watermark_lsb_embed(uint8_t *luma, int width, int height, int stride,
                        const watermark_payload_t *payload);

/**
 * watermark_lsb_extract — extract bit stream from 8-bit luma plane
 *
 * Uses the same viewer_id-seeded index sequence as embed to pick the
 * same pixels and read back the LSBs.
 *
 * @param luma       8-bit planar luma buffer (read-only)
 * @param width      Frame width in pixels
 * @param height     Frame height in pixels
 * @param stride     Bytes per row
 * @param viewer_id  Viewer ID used during embedding (for PRNG seed)
 * @param out        Output payload (viewer_id populated from bits)
 * @return           Number of bits extracted (64), or -1 on error
 */
int watermark_lsb_extract(const uint8_t *luma, int width, int height, int stride,
                          uint64_t viewer_id, watermark_payload_t *out);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_WATERMARK_LSB_H */
