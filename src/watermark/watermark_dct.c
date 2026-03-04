/*
 * watermark_dct.c — DCT-domain watermark embed/extract (sign substitution)
 *
 * Performs an exact 8×8 DCT-II and its inverse (IDCT-III) using the
 * standard orthonormal definition.  No external library needed.
 *
 * Embedding strategy: sign substitution at mid-frequency coefficient (3,4)
 *   - Bit 1 → set coefficient to +delta
 *   - Bit 0 → set coefficient to -delta
 *   - Extraction: coefficient > 0 → bit 1, else bit 0
 *
 * This is robust to IDCT→integer-round→re-DCT as long as delta > rounding
 * noise (~8 for 8×8 blocks of 8-bit pixels).  WATERMARK_DCT_DELTA_DEFAULT
 * is set to 32 which provides comfortable margin.
 */

#include "watermark_dct.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define BLK  8

/* ── 8×8 forward DCT ────────────────────────────────────────────── */

static void dct8x8(const float in[BLK*BLK], float out[BLK*BLK]) {
    float tmp[BLK*BLK];
    /* Row DCTs */
    for (int r = 0; r < BLK; r++) {
        for (int k = 0; k < BLK; k++) {
            float s = 0.0f;
            for (int n = 0; n < BLK; n++)
                s += in[r*BLK+n] * cosf((float)M_PI/(float)BLK * ((float)n+0.5f) * (float)k);
            tmp[r*BLK+k] = s;
        }
    }
    /* Column DCTs */
    for (int c = 0; c < BLK; c++) {
        for (int k = 0; k < BLK; k++) {
            float s = 0.0f;
            for (int n = 0; n < BLK; n++)
                s += tmp[n*BLK+c] * cosf((float)M_PI/(float)BLK * ((float)n+0.5f) * (float)k);
            out[k*BLK+c] = s;
        }
    }
}

/* ── 8×8 inverse DCT ────────────────────────────────────────────── */

static void idct8x8(const float in[BLK*BLK], float out[BLK*BLK]) {
    float tmp[BLK*BLK];
    /* Column IDCTs */
    for (int c = 0; c < BLK; c++) {
        for (int n = 0; n < BLK; n++) {
            float s = in[0*BLK+c];
            for (int k = 1; k < BLK; k++)
                s += 2.0f * in[k*BLK+c] * cosf((float)M_PI/(float)BLK * ((float)n+0.5f) * (float)k);
            tmp[n*BLK+c] = s / (float)BLK;
        }
    }
    /* Row IDCTs */
    for (int r = 0; r < BLK; r++) {
        for (int n = 0; n < BLK; n++) {
            float s = tmp[r*BLK+0];
            for (int k = 1; k < BLK; k++)
                s += 2.0f * tmp[r*BLK+k] * cosf((float)M_PI/(float)BLK * ((float)n+0.5f) * (float)k);
            out[r*BLK+n] = s / (float)BLK;
        }
    }
}

/* ── Block I/O helpers ───────────────────────────────────────────── */

static void read_block(const uint8_t *luma, int stride,
                        int bx, int by, float blk[BLK*BLK]) {
    for (int r = 0; r < BLK; r++)
        for (int c = 0; c < BLK; c++)
            blk[r*BLK+c] = (float)luma[(by*BLK+r)*stride + bx*BLK+c];
}

static void write_block(uint8_t *luma, int stride,
                         int bx, int by, const float blk[BLK*BLK]) {
    for (int r = 0; r < BLK; r++) {
        for (int c = 0; c < BLK; c++) {
            float v = blk[r*BLK+c];
            if (v < 0.0f) v = 0.0f;
            if (v > 255.0f) v = 255.0f;
            luma[(by*BLK+r)*stride + bx*BLK+c] = (uint8_t)(v + 0.5f);
        }
    }
}

/* Mid-frequency coefficient position (3,4) used per block */
#define MF_ROW 3
#define MF_COL 4

/* ── Public API ─────────────────────────────────────────────────── */

int watermark_dct_embed(uint8_t                   *luma,
                          int                        width,
                          int                        height,
                          int                        stride,
                          const watermark_payload_t *payload,
                          int                        delta) {
    if (!luma || !payload || width < BLK*64 || height < BLK || stride < width) return -1;

    uint8_t bits[64];
    int n = watermark_payload_to_bits(payload, bits, 64);
    if (n < 0) return -1;

    int blocks_per_row = width / BLK;
    float blk[BLK*BLK], dct[BLK*BLK], idct[BLK*BLK];

    for (int b = 0; b < 64; b++) {
        int bx = b % blocks_per_row;
        int by = b / blocks_per_row;
        if (by * BLK >= height) break;

        read_block(luma, stride, bx, by, blk);
        dct8x8(blk, dct);
        /*
         * Sign substitution: set coefficient to +delta (bit=1) or -delta (bit=0).
         * Robust because |signal| = delta >> rounding_noise (~8 for 8×8 blocks).
         */
        dct[MF_ROW*BLK+MF_COL] = (float)(bits[b] ? delta : -delta);
        idct8x8(dct, idct);
        write_block(luma, stride, bx, by, idct);
    }
    return 64;
}

int watermark_dct_extract(const uint8_t       *luma,
                            int                  width,
                            int                  height,
                            int                  stride,
                            int                  delta,
                            watermark_payload_t  *out) {
    if (!luma || !out || width < BLK*64 || height < BLK || stride < width) return -1;
    (void)delta; /* kept in signature for API compatibility; not needed for sign check */

    int blocks_per_row = width / BLK;
    float blk[BLK*BLK], dct[BLK*BLK];
    uint8_t bits[64];

    for (int b = 0; b < 64; b++) {
        int bx = b % blocks_per_row;
        int by = b / blocks_per_row;
        if (by * BLK >= height) break;

        read_block(luma, stride, bx, by, blk);
        dct8x8(blk, dct);
        /* Bit = 1 if coefficient > 0, 0 otherwise */
        bits[b] = (dct[MF_ROW*BLK+MF_COL] > 0.0f) ? 1u : 0u;
    }

    memset(out, 0, sizeof(*out));
    if (watermark_payload_from_bits(bits, 64, out) != 0) return -1;
    return 64;
}

