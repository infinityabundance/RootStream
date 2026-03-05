/*
 * phash.c — Perceptual hash implementation (DCT-based, 64-bit)
 *
 * Uses a separable 1D DCT-II over a 32×32 down-scaled luma image.
 * No external math library needed; uses only standard C.
 */

#include "phash.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ── Bilinear resize to 32×32 ─────────────────────────────────── */

static void resize_bilinear(const uint8_t *src,
                              int            src_w,
                              int            src_h,
                              int            src_stride,
                              float         *dst) {
    for (int dy = 0; dy < PHASH_WORK_SIZE; dy++) {
        float sy = (float)dy * (float)(src_h - 1) / (float)(PHASH_WORK_SIZE - 1);
        int   y0 = (int)sy;
        int   y1 = y0 + 1;
        if (y1 >= src_h) y1 = src_h - 1;
        float fy = sy - (float)y0;

        for (int dx = 0; dx < PHASH_WORK_SIZE; dx++) {
            float sx = (float)dx * (float)(src_w - 1) / (float)(PHASH_WORK_SIZE - 1);
            int   x0 = (int)sx;
            int   x1 = x0 + 1;
            if (x1 >= src_w) x1 = src_w - 1;
            float fx = sx - (float)x0;

            float p00 = (float)src[y0 * src_stride + x0];
            float p01 = (float)src[y0 * src_stride + x1];
            float p10 = (float)src[y1 * src_stride + x0];
            float p11 = (float)src[y1 * src_stride + x1];

            dst[dy * PHASH_WORK_SIZE + dx] =
                p00 * (1.0f - fx) * (1.0f - fy) +
                p01 * fx          * (1.0f - fy) +
                p10 * (1.0f - fx) * fy          +
                p11 * fx          * fy;
        }
    }
}

/* ── Separable 2D DCT-II over 32×32 grid ─────────────────────── */

static void dct1d(float *v, int n) {
    /* In-place 1D DCT-II: X[k] = sum_{n=0}^{N-1} x[n]*cos(pi*(n+0.5)*k/N) */
    float tmp[PHASH_WORK_SIZE];
    float pi_over_n = (float)M_PI / (float)n;
    for (int k = 0; k < n; k++) {
        float acc = 0.0f;
        for (int i = 0; i < n; i++)
            acc += v[i] * cosf(pi_over_n * ((float)i + 0.5f) * (float)k);
        tmp[k] = acc;
    }
    memcpy(v, tmp, sizeof(float) * (size_t)n);
}

static void dct2d(float *grid) {
    /* Row DCTs */
    for (int r = 0; r < PHASH_WORK_SIZE; r++)
        dct1d(grid + r * PHASH_WORK_SIZE, PHASH_WORK_SIZE);

    /* Column DCTs */
    float col[PHASH_WORK_SIZE];
    for (int c = 0; c < PHASH_WORK_SIZE; c++) {
        for (int r = 0; r < PHASH_WORK_SIZE; r++)
            col[r] = grid[r * PHASH_WORK_SIZE + c];
        dct1d(col, PHASH_WORK_SIZE);
        for (int r = 0; r < PHASH_WORK_SIZE; r++)
            grid[r * PHASH_WORK_SIZE + c] = col[r];
    }
}

/* ── Public API ────────────────────────────────────────────────── */

int phash_compute(const uint8_t *luma,
                   int            width,
                   int            height,
                   int            stride,
                   uint64_t      *out) {
    if (!luma || !out || width <= 0 || height <= 0 || stride < width)
        return -1;

    float grid[PHASH_WORK_SIZE * PHASH_WORK_SIZE];
    resize_bilinear(luma, width, height, stride, grid);
    dct2d(grid);

    /* Extract top-left PHASH_FEAT_SIZE × PHASH_FEAT_SIZE, skip DC [0,0] */
    float feat[PHASH_BITS];
    int   idx = 0;
    for (int r = 0; r < PHASH_FEAT_SIZE; r++) {
        for (int c = 0; c < PHASH_FEAT_SIZE; c++) {
            if (r == 0 && c == 0) {
                feat[idx++] = 0.0f; /* DC placeholder, excluded from mean */
                continue;
            }
            feat[idx++] = grid[r * PHASH_WORK_SIZE + c];
        }
    }

    /* Mean of non-DC components */
    float mean = 0.0f;
    for (int i = 1; i < PHASH_BITS; i++) mean += feat[i];
    mean /= (float)(PHASH_BITS - 1);

    /* Build hash: bit i = (feat[i] > mean) */
    uint64_t hash = 0;
    for (int i = 0; i < PHASH_BITS; i++) {
        if (i == 0) continue; /* skip DC bit */
        if (feat[i] > mean)
            hash |= (1ULL << i);
    }

    *out = hash;
    return 0;
}

int phash_hamming(uint64_t a, uint64_t b) {
    /* Count bits set in XOR */
    uint64_t diff = a ^ b;
    int count = 0;
    while (diff) {
        count += (int)(diff & 1);
        diff >>= 1;
    }
    return count;
}

bool phash_similar(uint64_t a, uint64_t b, int max_dist) {
    return phash_hamming(a, b) <= max_dist;
}
