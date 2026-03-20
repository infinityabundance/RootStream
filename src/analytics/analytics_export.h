/*
 * analytics_export.h — JSON / CSV flush of analytics data
 *
 * Renders analytics snapshots and event batches into caller-supplied
 * buffers.  No heap allocations are performed.
 *
 * Thread-safety: all functions are stateless and thread-safe.
 */

#ifndef ROOTSTREAM_ANALYTICS_EXPORT_H
#define ROOTSTREAM_ANALYTICS_EXPORT_H

#include <stddef.h>

#include "analytics_event.h"
#include "analytics_stats.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * analytics_export_stats_json — render @stats as compact JSON into @buf
 *
 * Example output:
 *   {"stream_start_us":1700000000000000,"current_viewers":12,...}
 *
 * @param stats    Stats snapshot to render
 * @param buf      Output buffer
 * @param buf_sz   Buffer size in bytes
 * @return         Bytes written (excl. NUL), or -1 if buf too small
 */
int analytics_export_stats_json(const analytics_stats_t *stats, char *buf, size_t buf_sz);

/**
 * analytics_export_events_json — render @n events as a JSON array into @buf
 *
 * Example output:
 *   [{"ts":1700000,"type":"viewer_join","session":1,"value":0},...]
 *
 * @param events   Array of events to render
 * @param n        Number of events
 * @param buf      Output buffer
 * @param buf_sz   Buffer size
 * @return         Bytes written, or -1 if buf too small
 */
int analytics_export_events_json(const analytics_event_t *events, size_t n, char *buf,
                                 size_t buf_sz);

/**
 * analytics_export_events_csv — render @n events as CSV into @buf
 *
 * CSV header: timestamp_us,type,session_id,value,payload
 *
 * @param events   Array of events to render
 * @param n        Number of events
 * @param buf      Output buffer
 * @param buf_sz   Buffer size
 * @return         Bytes written, or -1 if buf too small
 */
int analytics_export_events_csv(const analytics_event_t *events, size_t n, char *buf,
                                size_t buf_sz);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_ANALYTICS_EXPORT_H */
