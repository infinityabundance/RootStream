/*
 * stream_metadata.c — Stream metadata encode/decode implementation
 */

#include "stream_metadata.h"

#include <stdio.h>
#include <string.h>

/* ── Little-endian helpers ──────────────────────────────────────── */

static void w16le(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}
static void w32le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}
static void w64le(uint8_t *p, uint64_t v) {
    for (int i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (i * 8));
}
static uint16_t r16le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t r32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t r64le(const uint8_t *p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= ((uint64_t)p[i] << (i * 8));
    return v;
}

/* Write a length-prefixed string into buf at *pos */
static int write_str(uint8_t *buf, size_t buf_sz, size_t *pos, const char *str, size_t max_len) {
    size_t slen = str ? strnlen(str, max_len) : 0;
    if (*pos + 2 + slen > buf_sz)
        return -1;
    w16le(buf + *pos, (uint16_t)slen);
    *pos += 2;
    if (slen) {
        memcpy(buf + *pos, str, slen);
        *pos += slen;
    }
    return 0;
}

/* Read a length-prefixed string from buf at *pos */
static int read_str(const uint8_t *buf, size_t buf_sz, size_t *pos, char *out, size_t out_max) {
    if (*pos + 2 > buf_sz)
        return -1;
    uint16_t slen = r16le(buf + *pos);
    *pos += 2;
    if (slen > out_max)
        return -1;
    if (*pos + slen > buf_sz)
        return -1;
    if (slen)
        memcpy(out, buf + *pos, slen);
    out[slen] = '\0';
    *pos += slen;
    return 0;
}

/* ── Public API ─────────────────────────────────────────────────── */

int stream_metadata_encode(const stream_metadata_t *meta, uint8_t *buf, size_t buf_sz) {
    if (!meta || !buf || buf_sz < METADATA_FIXED_HDR_SZ)
        return -1;

    size_t pos = 0;
    w32le(buf + pos, (uint32_t)METADATA_MAGIC);
    pos += 4;
    w64le(buf + pos, meta->start_us);
    pos += 8;
    w32le(buf + pos, meta->duration_us);
    pos += 4;
    w16le(buf + pos, meta->video_width);
    pos += 2;
    w16le(buf + pos, meta->video_height);
    pos += 2;
    buf[pos++] = meta->video_fps;
    buf[pos++] = meta->flags;

    if (write_str(buf, buf_sz, &pos, meta->title, METADATA_MAX_TITLE) != 0)
        return -1;
    if (write_str(buf, buf_sz, &pos, meta->description, METADATA_MAX_DESC) != 0)
        return -1;
    if (write_str(buf, buf_sz, &pos, meta->tags, METADATA_MAX_TAGS) != 0)
        return -1;

    return (int)pos;
}

int stream_metadata_decode(const uint8_t *buf, size_t buf_sz, stream_metadata_t *meta) {
    if (!buf || !meta || buf_sz < METADATA_FIXED_HDR_SZ)
        return -1;

    size_t pos = 0;
    if (r32le(buf) != (uint32_t)METADATA_MAGIC)
        return -1;
    pos += 4;

    memset(meta, 0, sizeof(*meta));
    meta->start_us = r64le(buf + pos);
    pos += 8;
    meta->duration_us = r32le(buf + pos);
    pos += 4;
    meta->video_width = r16le(buf + pos);
    pos += 2;
    meta->video_height = r16le(buf + pos);
    pos += 2;
    meta->video_fps = buf[pos++];
    meta->flags = buf[pos++];

    if (read_str(buf, buf_sz, &pos, meta->title, METADATA_MAX_TITLE) != 0)
        return -1;
    if (read_str(buf, buf_sz, &pos, meta->description, METADATA_MAX_DESC) != 0)
        return -1;
    if (read_str(buf, buf_sz, &pos, meta->tags, METADATA_MAX_TAGS) != 0)
        return -1;

    return 0;
}

bool stream_metadata_is_live(const stream_metadata_t *meta) {
    return meta ? !!(meta->flags & METADATA_FLAG_LIVE) : false;
}
