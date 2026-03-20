/*
 * jitter_packet.c — Jitter packet encode/decode/ordering implementation
 */

#include "jitter_packet.h"

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

int jitter_packet_encode(const jitter_packet_t *pkt, uint8_t *buf, size_t buf_sz) {
    if (!pkt || !buf)
        return -1;
    if (pkt->payload_len > JITTER_MAX_PAYLOAD)
        return -1;
    size_t needed = JITTER_PKT_HDR_SIZE + (size_t)pkt->payload_len;
    if (buf_sz < needed)
        return -1;

    w32le(buf + 0, (uint32_t)JITTER_MAGIC);
    w32le(buf + 4, pkt->seq_num);
    w32le(buf + 8, pkt->rtp_ts);
    w64le(buf + 12, pkt->capture_us);
    w16le(buf + 20, pkt->payload_len);
    buf[22] = pkt->payload_type;
    buf[23] = pkt->flags;
    if (pkt->payload_len > 0)
        memcpy(buf + JITTER_PKT_HDR_SIZE, pkt->payload, pkt->payload_len);
    return (int)needed;
}

int jitter_packet_decode(const uint8_t *buf, size_t buf_sz, jitter_packet_t *pkt) {
    if (!buf || !pkt || buf_sz < JITTER_PKT_HDR_SIZE)
        return -1;
    if (r32le(buf) != (uint32_t)JITTER_MAGIC)
        return -1;

    memset(pkt, 0, sizeof(*pkt));
    pkt->seq_num = r32le(buf + 4);
    pkt->rtp_ts = r32le(buf + 8);
    pkt->capture_us = r64le(buf + 12);
    pkt->payload_len = r16le(buf + 20);
    pkt->payload_type = buf[22];
    pkt->flags = buf[23];

    if (pkt->payload_len > JITTER_MAX_PAYLOAD)
        return -1;
    if (buf_sz < JITTER_PKT_HDR_SIZE + (size_t)pkt->payload_len)
        return -1;
    if (pkt->payload_len > 0)
        memcpy(pkt->payload, buf + JITTER_PKT_HDR_SIZE, pkt->payload_len);
    return 0;
}

bool jitter_packet_before(uint32_t a, uint32_t b) {
    /* RFC 3550 sequence comparison: half-window modular comparison.
     * a is strictly before b if their difference is in the forward half-window. */
    return a != b && (uint32_t)(b - a) < 0x80000000UL;
}
