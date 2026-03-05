/*
 * quality_monitor.h — Rolling quality monitor with alert thresholds
 *
 * Maintains a sliding window of per-frame quality scores (PSNR / SSIM)
 * and fires alert callbacks when averages drop below configurable
 * thresholds.
 *
 * Typical usage
 * ─────────────
 *   quality_monitor_t *m = quality_monitor_create(&cfg);
 *   // per frame:
 *   quality_monitor_push(m, psnr, ssim);
 *   if (quality_monitor_is_degraded(m)) { ... request keyframe ... }
 *   quality_monitor_get_stats(m, &stats);
 *   quality_monitor_destroy(m);
 */

#ifndef ROOTSTREAM_QUALITY_MONITOR_H
#define ROOTSTREAM_QUALITY_MONITOR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Rolling window capacity (frames) */
#define QUALITY_WINDOW_SIZE 120   /* 2 seconds at 60 fps */

/** Configuration */
typedef struct {
    double psnr_threshold;    /**< Min acceptable average PSNR (e.g. 30.0) */
    double ssim_threshold;    /**< Min acceptable average SSIM (e.g. 0.85) */
    int    window_size;       /**< Rolling window size (0 = use default) */
} quality_monitor_config_t;

/** Point-in-time quality statistics snapshot */
typedef struct {
    double  avg_psnr;         /**< Average PSNR over window */
    double  avg_ssim;         /**< Average SSIM over window */
    double  min_psnr;         /**< Minimum PSNR in window */
    double  min_ssim;         /**< Minimum SSIM in window */
    uint64_t frames_total;    /**< Total frames pushed since creation */
    uint64_t alerts_total;    /**< Total degradation alerts fired */
    bool     degraded;        /**< True if currently below threshold */
} quality_stats_t;

/** Opaque monitor handle */
typedef struct quality_monitor_s quality_monitor_t;

/**
 * quality_monitor_create — allocate monitor
 *
 * @param config  Configuration; NULL uses defaults (PSNR≥30, SSIM≥0.85)
 * @return        Non-NULL handle, or NULL on OOM
 */
quality_monitor_t *quality_monitor_create(const quality_monitor_config_t *config);

/**
 * quality_monitor_destroy — free all resources
 *
 * @param m  Monitor to destroy
 */
void quality_monitor_destroy(quality_monitor_t *m);

/**
 * quality_monitor_push — record quality scores for one frame
 *
 * @param m     Monitor
 * @param psnr  PSNR score for this frame (dB)
 * @param ssim  SSIM score for this frame [0,1]
 */
void quality_monitor_push(quality_monitor_t *m, double psnr, double ssim);

/**
 * quality_monitor_is_degraded — return true when average quality is below threshold
 *
 * @param m  Monitor
 * @return   true if quality is currently degraded
 */
bool quality_monitor_is_degraded(const quality_monitor_t *m);

/**
 * quality_monitor_get_stats — fill @stats with current window averages
 *
 * @param m      Monitor
 * @param stats  Output statistics
 */
void quality_monitor_get_stats(const quality_monitor_t *m,
                                quality_stats_t         *stats);

/**
 * quality_monitor_reset — clear all history
 *
 * @param m  Monitor
 */
void quality_monitor_reset(quality_monitor_t *m);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_QUALITY_MONITOR_H */
