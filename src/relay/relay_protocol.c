/*
 * relay_protocol.c — Relay wire protocol serialisation
 */

#include "relay_protocol.h"

#include <string.h>

/* ── Header encode / decode ─────────────────────────────────────── */

int relay_encode_header(const relay_header_t *hdr, uint8_t *buf) {
    if (!hdr || !buf) return -1;

    /* Magic (big-endian) */
    buf[0] = (uint8_t)(RELAY_MAGIC >> 8);
    buf[1] = (uint8_t)(RELAY_MAGIC & 0xFF);
    buf[2] = RELAY_VERSION;
    buf[3] = (uint8_t)hdr->type;

    /* Session ID (big-endian) */
    buf[4] = (uint8_t)(hdr->session_id >> 24);
    buf[5] = (uint8_t)(hdr->session_id >> 16);
    buf[6] = (uint8_t)(hdr->session_id >>  8);
    buf[7] = (uint8_t)(hdr->session_id       );

    /* Payload length (big-endian) */
    buf[8] = (uint8_t)(hdr->payload_len >> 8);
    buf[9] = (uint8_t)(hdr->payload_len     );

    return RELAY_HDR_SIZE;
}

int relay_decode_header(const uint8_t *buf, relay_header_t *hdr) {
    if (!buf || !hdr) return -1;

    uint16_t magic = ((uint16_t)buf[0] << 8) | buf[1];
    if (magic != RELAY_MAGIC) return -1;
    if (buf[2] != RELAY_VERSION) return -1;

    hdr->type        = (relay_msg_type_t)buf[3];
    hdr->session_id  = ((uint32_t)buf[4] << 24)
                     | ((uint32_t)buf[5] << 16)
                     | ((uint32_t)buf[6] <<  8)
                     |  (uint32_t)buf[7];
    hdr->payload_len = ((uint16_t)buf[8] << 8) | buf[9];

    return 0;
}

/* ── HELLO payload ───────────────────────────────────────────────── */

#define HELLO_PAYLOAD_LEN 36  /* 32-byte token + 1-byte role + 3 reserved */

int relay_build_hello(const uint8_t *token, bool is_host, uint8_t *buf) {
    if (!token || !buf) return -1;
    memcpy(buf, token, RELAY_TOKEN_LEN);
    buf[RELAY_TOKEN_LEN]     = is_host ? 0x00 : 0x01;
    buf[RELAY_TOKEN_LEN + 1] = 0;
    buf[RELAY_TOKEN_LEN + 2] = 0;
    buf[RELAY_TOKEN_LEN + 3] = 0;
    return HELLO_PAYLOAD_LEN;
}

int relay_parse_hello(const uint8_t *payload, uint16_t payload_len,
                      uint8_t *out_token, bool *out_is_host) {
    if (!payload || !out_token || !out_is_host) return -1;
    if (payload_len < HELLO_PAYLOAD_LEN) return -1;

    memcpy(out_token, payload, RELAY_TOKEN_LEN);
    *out_is_host = (payload[RELAY_TOKEN_LEN] == 0x00);
    return 0;
}
