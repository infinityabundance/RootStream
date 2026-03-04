/*
 * metadata_export.c — JSON export of stream metadata
 */

#include "metadata_export.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

int metadata_export_json(const stream_metadata_t *meta,
                           char                    *buf,
                           size_t                   buf_sz) {
    if (!meta || !buf || buf_sz == 0) return -1;

    int n = snprintf(buf, buf_sz,
        "{"
        "\"start_us\":%" PRIu64 ","
        "\"duration_us\":%" PRIu32 ","
        "\"video_width\":%" PRIu16 ","
        "\"video_height\":%" PRIu16 ","
        "\"video_fps\":%u,"
        "\"flags\":%u,"
        "\"live\":%s,"
        "\"title\":\"%s\","
        "\"description\":\"%s\","
        "\"tags\":\"%s\""
        "}",
        meta->start_us,
        meta->duration_us,
        meta->video_width,
        meta->video_height,
        (unsigned)meta->video_fps,
        (unsigned)meta->flags,
        (meta->flags & METADATA_FLAG_LIVE) ? "true" : "false",
        meta->title,
        meta->description,
        meta->tags);

    if (n < 0 || (size_t)n >= buf_sz) return -1;
    return n;
}

/* ── KV store JSON export ────────────────────────────────────────── */

typedef struct {
    char  *buf;
    size_t buf_sz;
    size_t pos;
    bool   first;
    bool   overflow;
} kv_json_ctx_t;

static int kv_json_cb(const char *key, const char *value, void *ud) {
    kv_json_ctx_t *ctx = (kv_json_ctx_t *)ud;
    if (ctx->overflow) return 1;

    int r = snprintf(ctx->buf + ctx->pos, ctx->buf_sz - ctx->pos,
                     "%s\"%s\":\"%s\"",
                     ctx->first ? "" : ",", key, value);
    if (r < 0 || (size_t)r >= ctx->buf_sz - ctx->pos) {
        ctx->overflow = true;
        return 1;
    }
    ctx->pos += (size_t)r;
    ctx->first = false;
    return 0;
}

int metadata_store_export_json(const metadata_store_t *store,
                                 char                   *buf,
                                 size_t                  buf_sz) {
    if (!store || !buf || buf_sz == 0) return -1;

    size_t pos = 0;
    int r = snprintf(buf + pos, buf_sz - pos, "{");
    if (r < 0 || (size_t)r >= buf_sz - pos) return -1;
    pos += (size_t)r;

    kv_json_ctx_t ctx = { buf, buf_sz, pos, true, false };
    metadata_store_foreach(store, kv_json_cb, &ctx);
    if (ctx.overflow) return -1;
    pos = ctx.pos;

    r = snprintf(buf + pos, buf_sz - pos, "}");
    if (r < 0 || (size_t)r >= buf_sz - pos) return -1;
    pos += (size_t)r;
    return (int)pos;
}

