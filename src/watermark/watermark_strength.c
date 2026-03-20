/*
 * watermark_strength.c — Adaptive watermark strength control implementation
 */

#include "watermark_strength.h"

#include "watermark_dct.h"

int watermark_strength_select(int quality_hint, bool is_keyframe, watermark_strength_t *out) {
    if (!out)
        return -1;

    out->apply = true;

    if (quality_hint < 0)
        quality_hint = 0;
    if (quality_hint > 100)
        quality_hint = 100;

    if (quality_hint >= 70) {
        /* High quality / lossless — use LSB (imperceptible) */
        out->mode = WATERMARK_MODE_LSB;
        out->dct_delta = 0;
    } else if (quality_hint >= 30) {
        /* Medium quality — DCT QIM with small delta */
        out->mode = WATERMARK_MODE_DCT;
        out->dct_delta = WATERMARK_DCT_DELTA_DEFAULT;
    } else {
        /* Low quality / heavy compression — DCT QIM with larger delta */
        out->mode = WATERMARK_MODE_DCT;
        out->dct_delta = WATERMARK_DCT_DELTA_DEFAULT * 2;
    }

    /* Only embed on keyframes for DCT mode to reduce computation */
    if (out->mode == WATERMARK_MODE_DCT && !is_keyframe) {
        out->apply = false;
    }

    return 0;
}

const char *watermark_strength_mode_name(watermark_mode_t mode) {
    switch (mode) {
        case WATERMARK_MODE_LSB:
            return "lsb";
        case WATERMARK_MODE_DCT:
            return "dct";
        default:
            return "unknown";
    }
}
