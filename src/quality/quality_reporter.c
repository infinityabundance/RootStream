/*
 * quality_reporter.c — JSON quality report generator
 */

#include "quality_reporter.h"

#include <stdio.h>
#include <string.h>

#define QUALITY_REPORT_MIN_SZ 256

size_t quality_report_min_buf_size(void) {
    return QUALITY_REPORT_MIN_SZ;
}

int quality_report_json(const quality_stats_t *stats,
                        uint64_t               scene_changes,
                        char                  *buf,
                        size_t                 buf_sz) {
    if (!stats || !buf || buf_sz == 0) return -1;

    int n = snprintf(buf, buf_sz,
        "{"
        "\"frames_total\":%llu,"
        "\"alerts_total\":%llu,"
        "\"degraded\":%s,"
        "\"psnr\":{"
          "\"avg\":%.4f,"
          "\"min\":%.4f"
        "},"
        "\"ssim\":{"
          "\"avg\":%.6f,"
          "\"min\":%.6f"
        "},"
        "\"scene_changes\":%llu"
        "}",
        (unsigned long long)stats->frames_total,
        (unsigned long long)stats->alerts_total,
        stats->degraded ? "true" : "false",
        stats->avg_psnr,
        stats->min_psnr,
        stats->avg_ssim,
        stats->min_ssim,
        (unsigned long long)scene_changes
    );

    if (n < 0 || (size_t)n >= buf_sz) return -1;
    return n;
}
