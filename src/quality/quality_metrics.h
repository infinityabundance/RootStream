/*
 * quality_metrics.h — Per-frame PSNR and SSIM quality scoring
 *
 * Computes two standard video-quality metrics on luma (Y) planes:
 *
 *   PSNR  (Peak Signal-to-Noise Ratio, dB)
 *         Higher is better.  Typical streaming targets: > 35 dB good,
 *         > 40 dB excellent.  Returns INFINITY when frames are identical.
 *
 *   SSIM  (Structural Similarity Index, 0.0–1.0)
 *         Higher is better.  > 0.95 is generally considered excellent.
 *
 * Both functions operate on 8-bit planar luma buffers.  Chroma is
 * intentionally excluded so the module compiles without colour-space
 * knowledge and operates on any grey or luma-only representation.
 *
 * Thread-safety: all functions are stateless and thread-safe.
 */

#ifndef ROOTSTREAM_QUALITY_METRICS_H
#define ROOTSTREAM_QUALITY_METRICS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * quality_psnr — compute PSNR between two luma frames (dB)
 *
 * @param ref      Reference (original) luma plane, row-major
 * @param dist     Distorted (encoded/decoded) luma plane, same layout
 * @param width    Frame width in pixels
 * @param height   Frame height in pixels
 * @param stride   Row stride in bytes (>= width)
 * @return         PSNR in dB; returns 1000.0 (sentinel for ∞) when MSE == 0
 */
double quality_psnr(const uint8_t *ref,
                    const uint8_t *dist,
                    int            width,
                    int            height,
                    int            stride);

/**
 * quality_ssim — compute SSIM between two luma frames (0.0–1.0)
 *
 * Uses an 8×8 block decomposition; final score is the mean over all blocks.
 *
 * @param ref      Reference luma plane
 * @param dist     Distorted luma plane
 * @param width    Frame width in pixels
 * @param height   Frame height in pixels
 * @param stride   Row stride in bytes
 * @return         SSIM index in [0, 1]
 */
double quality_ssim(const uint8_t *ref,
                    const uint8_t *dist,
                    int            width,
                    int            height,
                    int            stride);

/**
 * quality_mse — compute Mean Squared Error between two luma planes
 *
 * @param ref, dist, width, height, stride  Same semantics as above
 * @return  MSE (non-negative double)
 */
double quality_mse(const uint8_t *ref,
                   const uint8_t *dist,
                   int            width,
                   int            height,
                   int            stride);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_QUALITY_METRICS_H */
