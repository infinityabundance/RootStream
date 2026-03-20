/*
 * caption_event.c — Caption event encode/decode implementation
 */

#include "caption_event.h"

#include <string.h>

/* ── Little-endian helpers ─────────────────────────────────────── */

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

/* ── Public API ───────────────────────────────────────────────── */

size_t caption_event_encoded_size(const caption_event_t *event) {
    if (!event)
        return 0;
    return CAPTION_HDR_SIZE + (size_t)event->text_len;
}

int caption_event_encode(const caption_event_t *event, uint8_t *buf, size_t buf_sz) {
    if (!event || !buf)
        return -1;
    size_t needed = caption_event_encoded_size(event);
    if (buf_sz < needed)
        return -1;

    w32le(buf + 0, (uint32_t)CAPTION_MAGIC);
    w64le(buf + 4, event->pts_us);
    w32le(buf + 12, event->duration_us);
    buf[16] = event->flags;
    buf[17] = event->row;
    w16le(buf + 18, event->text_len);
    if (event->text_len > 0) {
        memcpy(buf + CAPTION_HDR_SIZE, event->text, event->text_len);
    }
    return (int)needed;
}

int caption_event_decode(const uint8_t *buf, size_t buf_sz, caption_event_t *event) {
    if (!buf || !event || buf_sz < CAPTION_HDR_SIZE)
        return -1;
    if (r32le(buf) != (uint32_t)CAPTION_MAGIC)
        return -1;

    memset(event, 0, sizeof(*event));
    event->pts_us = r64le(buf + 4);
    event->duration_us = r32le(buf + 12);
    event->flags = buf[16];
    event->row = buf[17];
    event->text_len = r16le(buf + 18);

    if (event->text_len > CAPTION_MAX_TEXT_BYTES)
        return -1;
    if (buf_sz < CAPTION_HDR_SIZE + (size_t)event->text_len)
        return -1;

    if (event->text_len > 0) {
        memcpy(event->text, buf + CAPTION_HDR_SIZE, event->text_len);
    }
    event->text[event->text_len] = '\0';
    return 0;
}

bool caption_event_is_active(const caption_event_t *event, uint64_t now_us) {
    if (!event)
        return false;
    return now_us >= event->pts_us && now_us < event->pts_us + event->duration_us;
}
