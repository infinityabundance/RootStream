/*
 * watermark_payload.c — Watermark payload encode/decode/bit-stream
 */

#include "watermark_payload.h"

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

/* ── Public API ─────────────────────────────────────────────────── */

int watermark_payload_encode(const watermark_payload_t *payload, uint8_t *buf, size_t buf_sz) {
    if (!payload || !buf)
        return -1;
    size_t needed = WATERMARK_HDR_SIZE + WATERMARK_MAX_DATA_BYTES;
    if (buf_sz < needed)
        return -1;

    w32le(buf + 0, (uint32_t)WATERMARK_MAGIC);
    w64le(buf + 4, payload->viewer_id);
    w64le(buf + 12, payload->session_id);
    w64le(buf + 20, payload->timestamp_us);
    w16le(buf + 28, payload->payload_bits);
    w16le(buf + 30, 0); /* reserved */
    memcpy(buf + WATERMARK_HDR_SIZE, payload->data, WATERMARK_MAX_DATA_BYTES);
    return (int)needed;
}

int watermark_payload_decode(const uint8_t *buf, size_t buf_sz, watermark_payload_t *payload) {
    size_t needed = WATERMARK_HDR_SIZE + WATERMARK_MAX_DATA_BYTES;
    if (!buf || !payload || buf_sz < needed)
        return -1;
    if (r32le(buf) != (uint32_t)WATERMARK_MAGIC)
        return -1;

    memset(payload, 0, sizeof(*payload));
    payload->viewer_id = r64le(buf + 4);
    payload->session_id = r64le(buf + 12);
    payload->timestamp_us = r64le(buf + 20);
    payload->payload_bits = r16le(buf + 28);
    memcpy(payload->data, buf + WATERMARK_HDR_SIZE, WATERMARK_MAX_DATA_BYTES);
    return 0;
}

int watermark_payload_to_bits(const watermark_payload_t *payload, uint8_t *bits, size_t max_bits) {
    if (!payload || !bits || max_bits < 64)
        return -1;
    for (int i = 0; i < 64; i++) bits[i] = (uint8_t)((payload->viewer_id >> (63 - i)) & 1);
    return 64;
}

int watermark_payload_from_bits(const uint8_t *bits, int n_bits, watermark_payload_t *payload) {
    if (!bits || !payload || n_bits != 64)
        return -1;
    uint64_t id = 0;
    for (int i = 0; i < 64; i++) {
        id <<= 1;
        id |= (bits[i] & 1);
    }
    payload->viewer_id = id;
    return 0;
}
