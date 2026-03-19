/*
 * kfr_message.c — Keyframe request message encode/decode
 */

#include "kfr_message.h"

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

int kfr_message_encode(const kfr_message_t *msg, uint8_t *buf, size_t buf_sz) {
    if (!msg || !buf || buf_sz < KFR_MSG_SIZE)
        return -1;
    if (msg->type != KFR_TYPE_PLI && msg->type != KFR_TYPE_FIR)
        return -1;

    w32le(buf + 0, (uint32_t)KFR_MSG_MAGIC);
    buf[4] = (uint8_t)msg->type;
    buf[5] = msg->priority;
    w16le(buf + 6, msg->seq);
    w32le(buf + 8, msg->ssrc);
    w64le(buf + 12, msg->timestamp_us);
    w32le(buf + 20, 0); /* reserved */
    return KFR_MSG_SIZE;
}

int kfr_message_decode(const uint8_t *buf, size_t buf_sz, kfr_message_t *msg) {
    if (!buf || !msg || buf_sz < KFR_MSG_SIZE)
        return -1;
    if (r32le(buf) != (uint32_t)KFR_MSG_MAGIC)
        return -1;

    memset(msg, 0, sizeof(*msg));
    msg->type = (kfr_type_t)buf[4];
    msg->priority = buf[5];
    msg->seq = r16le(buf + 6);
    msg->ssrc = r32le(buf + 8);
    msg->timestamp_us = r64le(buf + 12);

    if (msg->type != KFR_TYPE_PLI && msg->type != KFR_TYPE_FIR)
        return -1;
    return 0;
}

const char *kfr_type_name(kfr_type_t t) {
    switch (t) {
        case KFR_TYPE_PLI:
            return "PLI";
        case KFR_TYPE_FIR:
            return "FIR";
        default:
            return "UNKNOWN";
    }
}
