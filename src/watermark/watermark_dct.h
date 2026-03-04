/*
 * watermark_dct.h — DCT-domain luma watermark embed/extract
 *
 * Embeds bits into the mid-frequency DCT coefficients of 8×8 luma
 * blocks.  Uses quantisation-index modulation (QIM): each bit is
 * represented by rounding a selected coefficient to the nearest even
 * or odd multiple of a step size Δ.
 *
 *   Embed bit b: c' = Δ * floor(c/Δ + 0.5 + b*0.5*(−1)^{floor(c/Δ)})
 *   Simplified:  even multiple → bit 0; odd multiple → bit 1
 *
 * The step size Δ controls the trade-off between robustness and PSNR:
 *   Δ = 4 → near-invisible (~50 dB PSNR); Δ = 8 → robust to re-encode
 *
 * Operates on an 8-bit luma plane; full 2D 8×8 DCT is performed
 * per block.  Only the coefficient at position (3,4) in each block
 * (mid-frequency) is used to carry one bit, cycling through 64 blocks
 * to embed 64 bits total.
 *
 * Thread-safety: stateless and thread-safe.
 */

#ifndef ROOTSTREAM_WATERMARK_DCT_H
#define ROOTSTREAM_WATERMARK_DCT_H

#include "watermark_payload.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Default QIM step size (chosen to exceed rounding noise after IDCT→round→DCT) */
#define WATERMARK_DCT_DELTA_DEFAULT  32

/**
 * watermark_dct_embed — embed payload bits into luma plane via QIM (in-place)
 *
 * Requires at least 64 8×8 blocks (i.e. width >= 64, height >= 8).
 *
 * @param luma     8-bit luma plane (modified in-place)
 * @param width    Frame width in pixels
 * @param height   Frame height in pixels
 * @param stride   Bytes per row
 * @param payload  Payload to embed
 * @param delta    QIM step size (use WATERMARK_DCT_DELTA_DEFAULT)
 * @return         Bits embedded, or -1 on error
 */
int watermark_dct_embed(uint8_t                   *luma,
                          int                        width,
                          int                        height,
                          int                        stride,
                          const watermark_payload_t *payload,
                          int                        delta);

/**
 * watermark_dct_extract — extract payload bits from luma plane via QIM
 *
 * @param luma    8-bit luma plane (read-only)
 * @param width   Frame width in pixels
 * @param height  Frame height in pixels
 * @param stride  Bytes per row
 * @param delta   QIM step size used during embedding
 * @param out     Output payload (viewer_id populated from bits)
 * @return        Bits extracted, or -1 on error
 */
int watermark_dct_extract(const uint8_t       *luma,
                            int                  width,
                            int                  height,
                            int                  stride,
                            int                  delta,
                            watermark_payload_t  *out);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_WATERMARK_DCT_H */
