/*
 * analytics_stats.h — Aggregate viewer and stream statistics collector
 *
 * Maintains running totals and averages from ingested analytics events.
 * All statistics are computed incrementally; no raw event log is kept.
 *
 * Thread-safety: NOT thread-safe.  Callers must synchronise if events
 * arrive on multiple threads.
 */

#ifndef ROOTSTREAM_ANALYTICS_STATS_H
#define ROOTSTREAM_ANALYTICS_STATS_H

#include <stddef.h>

#include "analytics_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Snapshot of aggregate stream statistics */
typedef struct {
    uint64_t stream_start_us;     /**< Timestamp of last stream start */
    uint64_t stream_stop_us;      /**< Timestamp of last stream stop */
    uint64_t total_viewer_joins;  /**< Cumulative viewer join events */
    uint64_t total_viewer_leaves; /**< Cumulative viewer leave events */
    int64_t current_viewers;      /**< Estimated concurrent viewers */
    uint64_t peak_viewers;        /**< Max concurrent viewers observed */
    uint64_t total_frame_drops;   /**< Cumulative frame-drop value sum */
    uint64_t quality_alerts;      /**< Cumulative quality alert count */
    uint64_t scene_changes;       /**< Scene change count since stream start */
    uint64_t latency_samples;     /**< Number of latency samples ingested */
    double avg_latency_us;        /**< Running average latency (µs) */
    double avg_bitrate_kbps;      /**< Running average bitrate (kbps) */
    uint64_t bitrate_samples;     /**< Number of bitrate samples */
} analytics_stats_t;

/** Opaque statistics handle */
typedef struct analytics_stats_s analytics_stats_ctx_t;

/**
 * analytics_stats_create — allocate statistics context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
analytics_stats_ctx_t *analytics_stats_create(void);

/**
 * analytics_stats_destroy — free context
 *
 * @param ctx  Context to destroy
 */
void analytics_stats_destroy(analytics_stats_ctx_t *ctx);

/**
 * analytics_stats_ingest — update statistics from @event
 *
 * @param ctx    Statistics context
 * @param event  Event to process
 * @return       0 on success, -1 on NULL args
 */
int analytics_stats_ingest(analytics_stats_ctx_t *ctx, const analytics_event_t *event);

/**
 * analytics_stats_snapshot — copy current statistics into @out
 *
 * @param ctx  Context
 * @param out  Destination snapshot
 * @return     0 on success, -1 on NULL args
 */
int analytics_stats_snapshot(const analytics_stats_ctx_t *ctx, analytics_stats_t *out);

/**
 * analytics_stats_reset — clear all accumulators
 *
 * @param ctx  Context to reset
 */
void analytics_stats_reset(analytics_stats_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_ANALYTICS_STATS_H */
