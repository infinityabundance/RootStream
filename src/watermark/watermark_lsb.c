/*
 * watermark_lsb.c — Spatial LSB watermark embed/extract implementation
 *
 * PRNG: xorshift64 seeded with viewer_id ^ 0xDEADBEEFCAFEBABE.
 * Pixel positions are drawn from [0, width*height) without replacement
 * using a simple modular walk — good enough for forensic watermarking.
 */

#include "watermark_lsb.h"

#include <string.h>

#define WM_BITS 64

/* ── xorshift64 PRNG ─────────────────────────────────────────────── */

static uint64_t xs64_next(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

/* ── Pixel index sequence ────────────────────────────────────────── */

static int pixel_index(uint64_t *rng, int total) {
    /* Rejection-sample to avoid modulo bias for large total */
    uint64_t v;
    do {
        v = xs64_next(rng);
    } while (v >= (uint64_t)(((uint64_t)(-1) / (unsigned)total) * (unsigned)total));
    return (int)(v % (unsigned)total);
}

/* ── Public API ─────────────────────────────────────────────────── */

int watermark_lsb_embed(uint8_t *luma, int width, int height, int stride,
                        const watermark_payload_t *payload) {
    if (!luma || !payload || width <= 0 || height <= 0 || stride < width)
        return -1;
    if (width * height < WM_BITS)
        return -1;

    uint8_t bits[WM_BITS];
    int n = watermark_payload_to_bits(payload, bits, WM_BITS);
    if (n < 0)
        return -1;

    uint64_t rng = payload->viewer_id ^ 0xDEADBEEFCAFEBABEULL;
    if (rng == 0)
        rng = 1;
    int total = width * height;

    for (int i = 0; i < WM_BITS; i++) {
        int idx = pixel_index(&rng, total);
        int row = idx / width;
        int col = idx % width;
        uint8_t orig = luma[row * stride + col];
        /* Embed bit: clear LSB and set to watermark bit */
        luma[row * stride + col] = (uint8_t)((orig & 0xFE) | (bits[i] & 1));
    }
    return WM_BITS;
}

int watermark_lsb_extract(const uint8_t *luma, int width, int height, int stride,
                          uint64_t viewer_id, watermark_payload_t *out) {
    if (!luma || !out || width <= 0 || height <= 0 || stride < width)
        return -1;
    if (width * height < WM_BITS)
        return -1;

    uint64_t rng = viewer_id ^ 0xDEADBEEFCAFEBABEULL;
    if (rng == 0)
        rng = 1;
    int total = width * height;

    uint8_t bits[WM_BITS];
    for (int i = 0; i < WM_BITS; i++) {
        int idx = pixel_index(&rng, total);
        int row = idx / width;
        int col = idx % width;
        bits[i] = luma[row * stride + col] & 1;
    }

    memset(out, 0, sizeof(*out));
    if (watermark_payload_from_bits(bits, WM_BITS, out) != 0)
        return -1;
    return WM_BITS;
}
