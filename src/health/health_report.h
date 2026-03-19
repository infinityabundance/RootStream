/*
 * health_report.h — JSON-style health snapshot serialiser
 *
 * Produces a compact, human-readable JSON string describing the current
 * state of all registered metrics in the monitor.  The output format is:
 *
 * {
 *   "overall": "OK",
 *   "n_ok": 3, "n_warn": 1, "n_crit": 0,
 *   "metrics": [
 *     { "name": "cpu_pct",  "kind": "GAUGE",   "level": "WARN", "value": 85.3 },
 *     { "name": "enc_running", "kind": "BOOLEAN", "level": "OK",   "value": 1 }
 *   ]
 * }
 *
 * The output is a NUL-terminated string written into a caller-supplied
 * buffer of at least @buf_size bytes.  If the buffer is too small the
 * output is truncated (guaranteed NUL-terminated).
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_HEALTH_REPORT_H
#define ROOTSTREAM_HEALTH_REPORT_H

#include <stddef.h>

#include "health_monitor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * health_report_json — serialise monitor state to JSON string
 *
 * @param hm      Monitor
 * @param buf     Output buffer
 * @param buf_sz  Buffer size (bytes)
 * @return        Number of bytes written (excl. NUL), or -1 on error
 */
int health_report_json(health_monitor_t *hm, char *buf, size_t buf_sz);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_HEALTH_REPORT_H */
