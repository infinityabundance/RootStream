/*
 * plc_frame.c — PCM audio frame encode/decode implementation
 */

#include "plc_frame.h"

#include <string.h>

/* ── Little-endian helpers ──────────────────────────────────────── */

static void w16le(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}
static void w32le(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
static void w64le(uint8_t *p, uint64_t v) {
    for (int i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (i * 8));
}
static uint16_t r16le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t r32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t r64le(const uint8_t *p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= ((uint64_t)p[i] << (i * 8));
    return v;
}

/* ── Public API ─────────────────────────────────────────────────── */

int plc_frame_byte_size(const plc_frame_t *frame) {
    if (!frame) return -1;
    return (int)(PLC_FRAME_HDR_SIZE +
                 (size_t)frame->channels * (size_t)frame->num_samples * 2);
}

int plc_frame_encode(const plc_frame_t *frame,
                      uint8_t           *buf,
                      size_t             buf_sz) {
    if (!frame || !buf) return -1;
    if (frame->channels == 0 || frame->channels > PLC_MAX_CHANNELS) return -1;
    if (frame->num_samples == 0 || frame->num_samples > PLC_MAX_SAMPLES_PER_CH) return -1;

    int sz = plc_frame_byte_size(frame);
    if (sz < 0 || buf_sz < (size_t)sz) return -1;

    w32le(buf + 0,  (uint32_t)PLC_FRAME_MAGIC);
    w64le(buf + 4,  frame->timestamp_us);
    w32le(buf + 12, frame->seq_num);
    w32le(buf + 16, frame->sample_rate);
    w16le(buf + 20, frame->channels);
    w16le(buf + 22, frame->num_samples);

    size_t n_samples = (size_t)frame->channels * (size_t)frame->num_samples;
    for (size_t i = 0; i < n_samples; i++) {
        w16le(buf + PLC_FRAME_HDR_SIZE + i * 2, (uint16_t)frame->samples[i]);
    }
    return sz;
}

int plc_frame_decode(const uint8_t *buf,
                      size_t         buf_sz,
                      plc_frame_t   *frame) {
    if (!buf || !frame || buf_sz < PLC_FRAME_HDR_SIZE) return -1;
    if (r32le(buf) != (uint32_t)PLC_FRAME_MAGIC) return -1;

    memset(frame, 0, sizeof(*frame));
    frame->timestamp_us = r64le(buf + 4);
    frame->seq_num      = r32le(buf + 12);
    frame->sample_rate  = r32le(buf + 16);
    frame->channels     = r16le(buf + 20);
    frame->num_samples  = r16le(buf + 22);

    if (frame->channels == 0 || frame->channels > PLC_MAX_CHANNELS) return -1;
    if (frame->num_samples == 0 || frame->num_samples > PLC_MAX_SAMPLES_PER_CH) return -1;

    size_t n_samples = (size_t)frame->channels * (size_t)frame->num_samples;
    if (buf_sz < PLC_FRAME_HDR_SIZE + n_samples * 2) return -1;

    for (size_t i = 0; i < n_samples; i++) {
        frame->samples[i] = (int16_t)r16le(buf + PLC_FRAME_HDR_SIZE + i * 2);
    }
    return 0;
}

bool plc_frame_is_silent(const plc_frame_t *frame) {
    if (!frame) return true;
    size_t n = (size_t)frame->channels * (size_t)frame->num_samples;
    for (size_t i = 0; i < n; i++) {
        if (frame->samples[i] != 0) return false;
    }
    return true;
}
