/*
 * analytics_stats.c — Aggregate statistics implementation
 */

#include "analytics_stats.h"

#include <stdlib.h>
#include <string.h>

struct analytics_stats_s {
    analytics_stats_t st;
};

analytics_stats_ctx_t *analytics_stats_create(void) {
    analytics_stats_ctx_t *ctx = calloc(1, sizeof(*ctx));
    return ctx;
}

void analytics_stats_destroy(analytics_stats_ctx_t *ctx) {
    free(ctx);
}

void analytics_stats_reset(analytics_stats_ctx_t *ctx) {
    if (ctx) memset(&ctx->st, 0, sizeof(ctx->st));
}

int analytics_stats_ingest(analytics_stats_ctx_t   *ctx,
                             const analytics_event_t *event) {
    if (!ctx || !event) return -1;
    analytics_stats_t *s = &ctx->st;

    switch (event->type) {
    case ANALYTICS_STREAM_START:
        s->stream_start_us = event->timestamp_us;
        s->scene_changes   = 0;
        break;
    case ANALYTICS_STREAM_STOP:
        s->stream_stop_us = event->timestamp_us;
        break;
    case ANALYTICS_VIEWER_JOIN:
        s->total_viewer_joins++;
        s->current_viewers++;
        if ((uint64_t)s->current_viewers > s->peak_viewers)
            s->peak_viewers = (uint64_t)s->current_viewers;
        break;
    case ANALYTICS_VIEWER_LEAVE:
        s->total_viewer_leaves++;
        if (s->current_viewers > 0) s->current_viewers--;
        break;
    case ANALYTICS_FRAME_DROP:
        s->total_frame_drops += event->value;
        break;
    case ANALYTICS_QUALITY_ALERT:
        s->quality_alerts++;
        break;
    case ANALYTICS_SCENE_CHANGE:
        s->scene_changes++;
        break;
    case ANALYTICS_LATENCY_SAMPLE: {
        /* Welford running mean */
        s->latency_samples++;
        double delta = (double)event->value - s->avg_latency_us;
        s->avg_latency_us += delta / (double)s->latency_samples;
        break;
    }
    case ANALYTICS_BITRATE_CHANGE: {
        s->bitrate_samples++;
        double delta = (double)event->value - s->avg_bitrate_kbps;
        s->avg_bitrate_kbps += delta / (double)s->bitrate_samples;
        break;
    }
    default:
        break;
    }
    return 0;
}

int analytics_stats_snapshot(const analytics_stats_ctx_t *ctx,
                               analytics_stats_t           *out) {
    if (!ctx || !out) return -1;
    *out = ctx->st;
    return 0;
}
