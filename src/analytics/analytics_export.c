/*
 * analytics_export.c — JSON and CSV export implementation
 */

#include "analytics_export.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

/* ── JSON stats ─────────────────────────────────────────────────── */

int analytics_export_stats_json(const analytics_stats_t *stats,
                                  char                    *buf,
                                  size_t                   buf_sz) {
    if (!stats || !buf || buf_sz == 0) return -1;

    int n = snprintf(buf, buf_sz,
        "{"
        "\"stream_start_us\":%" PRIu64 ","
        "\"stream_stop_us\":%" PRIu64 ","
        "\"total_viewer_joins\":%" PRIu64 ","
        "\"total_viewer_leaves\":%" PRIu64 ","
        "\"current_viewers\":%" PRId64 ","
        "\"peak_viewers\":%" PRIu64 ","
        "\"total_frame_drops\":%" PRIu64 ","
        "\"quality_alerts\":%" PRIu64 ","
        "\"scene_changes\":%" PRIu64 ","
        "\"avg_latency_us\":%.2f,"
        "\"avg_bitrate_kbps\":%.2f"
        "}",
        stats->stream_start_us,
        stats->stream_stop_us,
        stats->total_viewer_joins,
        stats->total_viewer_leaves,
        stats->current_viewers,
        stats->peak_viewers,
        stats->total_frame_drops,
        stats->quality_alerts,
        stats->scene_changes,
        stats->avg_latency_us,
        stats->avg_bitrate_kbps);

    if (n < 0 || (size_t)n >= buf_sz) return -1;
    return n;
}

/* ── JSON events ────────────────────────────────────────────────── */

int analytics_export_events_json(const analytics_event_t *events,
                                   size_t                   n,
                                   char                    *buf,
                                   size_t                   buf_sz) {
    if (!events || !buf || buf_sz == 0) return -1;
    if (n == 0) {
        if (buf_sz < 3) return -1;
        buf[0] = '['; buf[1] = ']'; buf[2] = '\0';
        return 2;
    }

    size_t pos = 0;
    int r = snprintf(buf + pos, buf_sz - pos, "[");
    if (r < 0 || (size_t)r >= buf_sz - pos) return -1;
    pos += (size_t)r;

    for (size_t i = 0; i < n; i++) {
        const analytics_event_t *e = &events[i];
        r = snprintf(buf + pos, buf_sz - pos,
                     "%s{\"ts\":%" PRIu64
                     ",\"type\":\"%s\""
                     ",\"session\":%" PRIu64
                     ",\"value\":%" PRIu64
                     ",\"payload\":\"%s\"}",
                     (i > 0 ? "," : ""),
                     e->timestamp_us,
                     analytics_event_type_name(e->type),
                     e->session_id,
                     e->value,
                     e->payload);
        if (r < 0 || (size_t)r >= buf_sz - pos) return -1;
        pos += (size_t)r;
    }

    r = snprintf(buf + pos, buf_sz - pos, "]");
    if (r < 0 || (size_t)r >= buf_sz - pos) return -1;
    pos += (size_t)r;
    return (int)pos;
}

/* ── CSV events ─────────────────────────────────────────────────── */

int analytics_export_events_csv(const analytics_event_t *events,
                                  size_t                   n,
                                  char                    *buf,
                                  size_t                   buf_sz) {
    if (!events || !buf || buf_sz == 0) return -1;

    size_t pos = 0;
    int r = snprintf(buf + pos, buf_sz - pos,
                     "timestamp_us,type,session_id,value,payload\n");
    if (r < 0 || (size_t)r >= buf_sz - pos) return -1;
    pos += (size_t)r;

    for (size_t i = 0; i < n; i++) {
        const analytics_event_t *e = &events[i];
        r = snprintf(buf + pos, buf_sz - pos,
                     "%" PRIu64 ",%s,%" PRIu64 ",%" PRIu64 ",%s\n",
                     e->timestamp_us,
                     analytics_event_type_name(e->type),
                     e->session_id,
                     e->value,
                     e->payload);
        if (r < 0 || (size_t)r >= buf_sz - pos) return -1;
        pos += (size_t)r;
    }

    return (int)pos;
}
