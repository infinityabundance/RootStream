/*
 * probe_packet.c — Bandwidth probe packet encode / decode
 */

#include "probe_packet.h"

#include <string.h>

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

int probe_packet_encode(const probe_packet_t *pkt, uint8_t *buf, size_t buf_sz) {
    if (!pkt || !buf || buf_sz < PROBE_PKT_SIZE)
        return -1;
    w32le(buf + 0, (uint32_t)PROBE_PKT_MAGIC);
    w16le(buf + 4, pkt->seq);
    w16le(buf + 6, pkt->size_hint);
    w64le(buf + 8, pkt->send_ts_us);
    w32le(buf + 16, pkt->burst_id);
    w32le(buf + 20, pkt->burst_seq);
    memset(buf + 24, 0, 8); /* reserved */
    return PROBE_PKT_SIZE;
}

int probe_packet_decode(const uint8_t *buf, size_t buf_sz, probe_packet_t *pkt) {
    if (!buf || !pkt || buf_sz < PROBE_PKT_SIZE)
        return -1;
    if (r32le(buf) != (uint32_t)PROBE_PKT_MAGIC)
        return -1;
    memset(pkt, 0, sizeof(*pkt));
    pkt->seq = r16le(buf + 4);
    pkt->size_hint = r16le(buf + 6);
    pkt->send_ts_us = r64le(buf + 8);
    pkt->burst_id = r32le(buf + 16);
    pkt->burst_seq = r32le(buf + 20);
    return 0;
}
