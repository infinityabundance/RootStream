/*
 * watermark_strength.h — Adaptive watermark strength control
 *
 * Selects the most appropriate embedding mode and parameters based on
 * frame properties (scene activity, encoding quality) to balance
 * imperceptibility vs. robustness.
 *
 * Two strategies are provided:
 *
 *   WATERMARK_MODE_LSB  — Spatial LSB embedding (fastest, invisible,
 *                         fragile to re-encoding / heavy compression)
 *
 *   WATERMARK_MODE_DCT  — DCT-domain QIM embedding (robust to moderate
 *                         JPEG/H.264 re-encoding, slightly lower PSNR)
 *
 * `watermark_strength_select()` chooses the mode and delta based on
 * a caller-supplied quality hint (0–100).
 */

#ifndef ROOTSTREAM_WATERMARK_STRENGTH_H
#define ROOTSTREAM_WATERMARK_STRENGTH_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Embedding mode */
typedef enum {
    WATERMARK_MODE_LSB = 0, /**< Spatial LSB (invisible, fragile) */
    WATERMARK_MODE_DCT = 1, /**< DCT-domain QIM (robust) */
} watermark_mode_t;

/** Strength parameters returned by the selector */
typedef struct {
    watermark_mode_t mode;
    int dct_delta; /**< QIM step size (mode=DCT only) */
    bool apply;    /**< False = skip watermarking this frame */
} watermark_strength_t;

/**
 * watermark_strength_select — choose embedding parameters
 *
 * @param quality_hint  Encoding quality hint 0–100 (0=low, 100=lossless)
 * @param is_keyframe   True if this is an IDR/keyframe
 * @param out           Output strength parameters
 * @return              0 on success, -1 on NULL args
 */
int watermark_strength_select(int quality_hint, bool is_keyframe, watermark_strength_t *out);

/**
 * watermark_strength_mode_name — return human-readable mode name
 *
 * @param mode  Watermark mode
 * @return      Static string (never NULL)
 */
const char *watermark_strength_mode_name(watermark_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_WATERMARK_STRENGTH_H */
