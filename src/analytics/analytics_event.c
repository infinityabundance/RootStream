/*
 * analytics_event.c — Analytics event encode/decode/name implementation
 */

#include "analytics_event.h"

#include <string.h>

/* ── Little-endian helpers ─────────────────────────────────────── */

static void w16le(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}
static void w64le(uint8_t *p, uint64_t v) {
    for (int i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (i * 8));
}
static void w32le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
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

/* ── Public API ────────────────────────────────────────────────── */

size_t analytics_event_encoded_size(const analytics_event_t *event) {
    return event ? ANALYTICS_HDR_SIZE + (size_t)event->payload_len : 0;
}

int analytics_event_encode(const analytics_event_t *event, uint8_t *buf, size_t buf_sz) {
    if (!event || !buf)
        return -1;
    size_t needed = analytics_event_encoded_size(event);
    if (buf_sz < needed)
        return -1;

    w32le(buf + 0, (uint32_t)ANALYTICS_MAGIC);
    w64le(buf + 4, event->timestamp_us);
    buf[12] = (uint8_t)event->type;
    buf[13] = event->flags;
    w16le(buf + 14, event->payload_len);
    w64le(buf + 16, event->session_id);
    w64le(buf + 24, event->value);
    if (event->payload_len > 0)
        memcpy(buf + ANALYTICS_HDR_SIZE, event->payload, event->payload_len);
    return (int)needed;
}

int analytics_event_decode(const uint8_t *buf, size_t buf_sz, analytics_event_t *event) {
    if (!buf || !event || buf_sz < ANALYTICS_HDR_SIZE)
        return -1;
    if (r32le(buf) != (uint32_t)ANALYTICS_MAGIC)
        return -1;

    memset(event, 0, sizeof(*event));
    event->timestamp_us = r64le(buf + 4);
    event->type = (analytics_event_type_t)buf[12];
    event->flags = buf[13];
    event->payload_len = r16le(buf + 14);
    event->session_id = r64le(buf + 16);
    event->value = r64le(buf + 24);

    if (event->payload_len > ANALYTICS_MAX_PAYLOAD)
        return -1;
    if (buf_sz < ANALYTICS_HDR_SIZE + (size_t)event->payload_len)
        return -1;
    if (event->payload_len > 0)
        memcpy(event->payload, buf + ANALYTICS_HDR_SIZE, event->payload_len);
    event->payload[event->payload_len] = '\0';
    return 0;
}

const char *analytics_event_type_name(analytics_event_type_t type) {
    switch (type) {
        case ANALYTICS_VIEWER_JOIN:
            return "viewer_join";
        case ANALYTICS_VIEWER_LEAVE:
            return "viewer_leave";
        case ANALYTICS_BITRATE_CHANGE:
            return "bitrate_change";
        case ANALYTICS_FRAME_DROP:
            return "frame_drop";
        case ANALYTICS_QUALITY_ALERT:
            return "quality_alert";
        case ANALYTICS_SCENE_CHANGE:
            return "scene_change";
        case ANALYTICS_STREAM_START:
            return "stream_start";
        case ANALYTICS_STREAM_STOP:
            return "stream_stop";
        case ANALYTICS_LATENCY_SAMPLE:
            return "latency_sample";
        default:
            return "unknown";
    }
}
