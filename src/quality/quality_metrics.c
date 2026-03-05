/*
 * quality_metrics.c — PSNR / SSIM / MSE implementation
 */

#include "quality_metrics.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

/* ── MSE ─────────────────────────────────────────────────────────── */

double quality_mse(const uint8_t *ref,
                   const uint8_t *dist,
                   int            width,
                   int            height,
                   int            stride) {
    if (!ref || !dist || width <= 0 || height <= 0 || stride < width) {
        return 0.0;
    }

    double sum = 0.0;
    for (int y = 0; y < height; y++) {
        const uint8_t *r = ref  + y * stride;
        const uint8_t *d = dist + y * stride;
        for (int x = 0; x < width; x++) {
            double diff = (double)r[x] - (double)d[x];
            sum += diff * diff;
        }
    }
    return sum / ((double)width * (double)height);
}

/* ── PSNR ────────────────────────────────────────────────────────── */

double quality_psnr(const uint8_t *ref,
                    const uint8_t *dist,
                    int            width,
                    int            height,
                    int            stride) {
    double mse = quality_mse(ref, dist, width, height, stride);
    if (mse < 1e-10) {
        return 1000.0;  /* sentinel for identical frames */
    }
    return 10.0 * log10(255.0 * 255.0 / mse);
}

/* ── SSIM helpers ────────────────────────────────────────────────── */

#define SSIM_BLOCK  8
#define SSIM_C1     (6.5025)    /* (0.01 * 255)^2 */
#define SSIM_C2     (58.5225)   /* (0.03 * 255)^2 */

static double block_ssim(const uint8_t *ref,
                          const uint8_t *dist,
                          int rx, int ry,
                          int width, int height, int stride) {
    /* Clamp block to frame boundary */
    int bw = (rx + SSIM_BLOCK <= width)  ? SSIM_BLOCK : (width  - rx);
    int bh = (ry + SSIM_BLOCK <= height) ? SSIM_BLOCK : (height - ry);
    int n  = bw * bh;

    double sum_r = 0.0, sum_d = 0.0;
    double sum_rr = 0.0, sum_dd = 0.0, sum_rd = 0.0;

    for (int y = ry; y < ry + bh; y++) {
        for (int x = rx; x < rx + bw; x++) {
            double rv = ref [y * stride + x];
            double dv = dist[y * stride + x];
            sum_r  += rv;
            sum_d  += dv;
            sum_rr += rv * rv;
            sum_dd += dv * dv;
            sum_rd += rv * dv;
        }
    }

    double mu_r  = sum_r  / n;
    double mu_d  = sum_d  / n;
    double var_r = sum_rr / n - mu_r * mu_r;
    double var_d = sum_dd / n - mu_d * mu_d;
    double cov   = sum_rd / n - mu_r * mu_d;

    double num = (2.0 * mu_r * mu_d + SSIM_C1) * (2.0 * cov + SSIM_C2);
    double den = (mu_r * mu_r + mu_d * mu_d + SSIM_C1)
               * (var_r + var_d + SSIM_C2);

    return (den > 1e-15) ? (num / den) : 1.0;
}

/* ── SSIM ────────────────────────────────────────────────────────── */

double quality_ssim(const uint8_t *ref,
                    const uint8_t *dist,
                    int            width,
                    int            height,
                    int            stride) {
    if (!ref || !dist || width <= 0 || height <= 0 || stride < width) {
        return 0.0;
    }

    double total = 0.0;
    int    count = 0;

    for (int y = 0; y < height; y += SSIM_BLOCK) {
        for (int x = 0; x < width; x += SSIM_BLOCK) {
            total += block_ssim(ref, dist, x, y, width, height, stride);
            count++;
        }
    }

    return (count > 0) ? (total / count) : 1.0;
}
