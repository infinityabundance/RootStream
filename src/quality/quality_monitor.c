/*
 * quality_monitor.c — Rolling quality monitor implementation
 */

#include "quality_monitor.h"

#include <float.h>
#include <stdlib.h>
#include <string.h>

struct quality_monitor_s {
    double psnr_window[QUALITY_WINDOW_SIZE];
    double ssim_window[QUALITY_WINDOW_SIZE];
    int window_size;
    int head;
    int filled; /* samples valid in window */

    double psnr_threshold;
    double ssim_threshold;

    uint64_t frames_total;
    uint64_t alerts_total;
    bool degraded;
};

quality_monitor_t *quality_monitor_create(const quality_monitor_config_t *config) {
    quality_monitor_t *m = calloc(1, sizeof(*m));
    if (!m)
        return NULL;

    if (config) {
        m->psnr_threshold = config->psnr_threshold;
        m->ssim_threshold = config->ssim_threshold;
        m->window_size = (config->window_size > 0 && config->window_size <= QUALITY_WINDOW_SIZE)
                             ? config->window_size
                             : QUALITY_WINDOW_SIZE;
    } else {
        m->psnr_threshold = 30.0;
        m->ssim_threshold = 0.85;
        m->window_size = QUALITY_WINDOW_SIZE;
    }

    /* Initialise window with "perfect" scores so first frames look good */
    for (int i = 0; i < m->window_size; i++) {
        m->psnr_window[i] = 40.0;
        m->ssim_window[i] = 1.0;
    }
    m->filled = 0;
    return m;
}

void quality_monitor_destroy(quality_monitor_t *m) {
    free(m);
}

void quality_monitor_push(quality_monitor_t *m, double psnr, double ssim) {
    if (!m)
        return;

    m->psnr_window[m->head] = psnr;
    m->ssim_window[m->head] = ssim;
    m->head = (m->head + 1) % m->window_size;
    if (m->filled < m->window_size)
        m->filled++;
    m->frames_total++;

    /* Recompute averages */
    int n = m->filled;
    double sum_psnr = 0.0, sum_ssim = 0.0;
    for (int i = 0; i < n; i++) {
        sum_psnr += m->psnr_window[i];
        sum_ssim += m->ssim_window[i];
    }
    double avg_psnr = sum_psnr / n;
    double avg_ssim = sum_ssim / n;

    bool was_degraded = m->degraded;
    m->degraded = (avg_psnr < m->psnr_threshold || avg_ssim < m->ssim_threshold);
    if (m->degraded && !was_degraded) {
        m->alerts_total++;
    }
}

bool quality_monitor_is_degraded(const quality_monitor_t *m) {
    return m ? m->degraded : false;
}

void quality_monitor_get_stats(const quality_monitor_t *m, quality_stats_t *stats) {
    if (!m || !stats)
        return;

    int n = m->filled;
    if (n == 0) {
        memset(stats, 0, sizeof(*stats));
        return;
    }

    double sum_psnr = 0.0, sum_ssim = 0.0;
    double min_psnr = DBL_MAX, min_ssim = DBL_MAX;

    for (int i = 0; i < n; i++) {
        double p = m->psnr_window[i];
        double s = m->ssim_window[i];
        sum_psnr += p;
        sum_ssim += s;
        if (p < min_psnr)
            min_psnr = p;
        if (s < min_ssim)
            min_ssim = s;
    }

    stats->avg_psnr = sum_psnr / n;
    stats->avg_ssim = sum_ssim / n;
    stats->min_psnr = min_psnr;
    stats->min_ssim = min_ssim;
    stats->frames_total = m->frames_total;
    stats->alerts_total = m->alerts_total;
    stats->degraded = m->degraded;
}

void quality_monitor_reset(quality_monitor_t *m) {
    if (!m)
        return;
    m->head = 0;
    m->filled = 0;
    m->frames_total = 0;
    m->alerts_total = 0;
    m->degraded = false;
    for (int i = 0; i < m->window_size; i++) {
        m->psnr_window[i] = 40.0;
        m->ssim_window[i] = 1.0;
    }
}
