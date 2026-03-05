/*
 * quality_reporter.h — JSON quality report generation
 *
 * Builds a structured JSON summary from a quality_stats_t snapshot and
 * an optional scene-change count.  The report is written into a
 * caller-supplied buffer; no heap allocation is performed.
 *
 * Output format (pretty-printed example):
 * ────────────────────────────────────────
 * {
 *   "frames_total": 1800,
 *   "alerts_total": 3,
 *   "degraded": false,
 *   "psnr": {
 *     "avg": 38.42,
 *     "min": 31.07
 *   },
 *   "ssim": {
 *     "avg": 0.9712,
 *     "min": 0.8843
 *   },
 *   "scene_changes": 5
 * }
 */

#ifndef ROOTSTREAM_QUALITY_REPORTER_H
#define ROOTSTREAM_QUALITY_REPORTER_H

#include "quality_monitor.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * quality_report_json — serialise @stats into a JSON string
 *
 * @param stats          Quality statistics snapshot
 * @param scene_changes  Number of scene changes detected (0 if unknown)
 * @param buf            Output buffer
 * @param buf_sz         Size of @buf in bytes
 * @return               Number of bytes written (excluding NUL), or -1 if
 *                       the buffer is too small
 */
int quality_report_json(const quality_stats_t *stats,
                        uint64_t               scene_changes,
                        char                  *buf,
                        size_t                 buf_sz);

/**
 * quality_report_min_buf_size — return minimum buffer for quality_report_json
 *
 * @return  Byte count sufficient for any valid report
 */
size_t quality_report_min_buf_size(void);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_QUALITY_REPORTER_H */
