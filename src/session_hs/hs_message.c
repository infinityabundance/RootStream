/*
 * hs_message.c — Handshake PDU encode / decode with CRC32
 *
 * CRC-32 uses the standard Ethernet polynomial (0xEDB88320, reflected).
 */

#include "hs_message.h"

#include <string.h>

/* ── CRC-32 (Ethernet / ISO 3309) ──────────────────────────────── */

static uint32_t crc32_byte(uint32_t crc, uint8_t b) {
    crc ^= b;
    for (int i = 0; i < 8; i++) crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1));
    return crc;
}

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
static uint16_t r16le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t r32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* ── Public API ─────────────────────────────────────────────────── */

int hs_message_encode(const hs_message_t *msg, uint8_t *buf, size_t buf_sz) {
    if (!msg || !buf)
        return -1;
    if (msg->payload_len > HS_MAX_PAYLOAD)
        return -1;
    size_t total = (size_t)(HS_MSG_HDR_SIZE + msg->payload_len);
    if (buf_sz < total)
        return -1;

    w32le(buf + 0, (uint32_t)HS_MSG_MAGIC);
    w16le(buf + 4, (uint16_t)msg->type);
    w16le(buf + 6, msg->seq);
    w16le(buf + 8, msg->payload_len);
    w16le(buf + 10, 0); /* reserved */
    memcpy(buf + HS_MSG_HDR_SIZE, msg->payload, msg->payload_len);

    /* CRC over header bytes [0..11] concatenated with payload */
    uint32_t c = 0xFFFFFFFFU;
    for (int i = 0; i < 12; i++) c = crc32_byte(c, buf[i]);
    for (int i = 0; i < msg->payload_len; i++) c = crc32_byte(c, buf[HS_MSG_HDR_SIZE + i]);
    uint32_t crc = c ^ 0xFFFFFFFFU;
    w32le(buf + 12, crc);

    return (int)total;
}

int hs_message_decode(const uint8_t *buf, size_t buf_sz, hs_message_t *msg) {
    if (!buf || !msg || buf_sz < (size_t)HS_MSG_HDR_SIZE)
        return -1;
    if (r32le(buf) != (uint32_t)HS_MSG_MAGIC)
        return -1;

    uint16_t plen = r16le(buf + 8);
    if (plen > HS_MAX_PAYLOAD)
        return -1;
    if (buf_sz < (size_t)(HS_MSG_HDR_SIZE + plen))
        return -1;

    /* Verify CRC: computed over header[0..11] + payload at [HS_MSG_HDR_SIZE..] */
    uint32_t c = 0xFFFFFFFFU;
    for (int i = 0; i < 12; i++) c = crc32_byte(c, buf[i]);
    for (int i = 0; i < plen; i++) c = crc32_byte(c, buf[HS_MSG_HDR_SIZE + i]);
    uint32_t expected = c ^ 0xFFFFFFFFU;
    if (r32le(buf + 12) != expected)
        return -1;

    memset(msg, 0, sizeof(*msg));
    msg->type = (hs_msg_type_t)r16le(buf + 4);
    msg->seq = r16le(buf + 6);
    msg->payload_len = plen;
    memcpy(msg->payload, buf + HS_MSG_HDR_SIZE, plen);
    return 0;
}

const char *hs_msg_type_name(hs_msg_type_t t) {
    switch (t) {
        case HS_MSG_HELLO:
            return "HELLO";
        case HS_MSG_HELLO_ACK:
            return "HELLO_ACK";
        case HS_MSG_AUTH:
            return "AUTH";
        case HS_MSG_AUTH_ACK:
            return "AUTH_ACK";
        case HS_MSG_CONFIG:
            return "CONFIG";
        case HS_MSG_READY:
            return "READY";
        case HS_MSG_ERROR:
            return "ERROR";
        case HS_MSG_BYE:
            return "BYE";
        default:
            return "UNKNOWN";
    }
}
