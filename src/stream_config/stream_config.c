/*
 * stream_config.c — Stream configuration encode/decode/helpers
 */

#include "stream_config.h"

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
static uint16_t r16le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t r32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* ── Public API ─────────────────────────────────────────────────── */

int stream_config_encode(const stream_config_t *cfg, uint8_t *buf, size_t buf_sz) {
    if (!cfg || !buf || buf_sz < SCFG_HDR_SIZE)
        return -1;

    w32le(buf + 0, (uint32_t)SCFG_MAGIC);
    buf[4] = cfg->video_codec;
    buf[5] = cfg->audio_codec;
    w16le(buf + 6, cfg->video_width);
    w16le(buf + 8, cfg->video_height);
    buf[10] = cfg->video_fps;
    buf[11] = cfg->audio_channels;
    w32le(buf + 12, cfg->video_bitrate_kbps);
    w32le(buf + 16, cfg->audio_bitrate_kbps);
    w32le(buf + 20, cfg->audio_sample_rate);
    w16le(buf + 24, cfg->transport_port);
    buf[26] = cfg->transport_proto;
    buf[27] = cfg->flags;
    w32le(buf + 28, 0); /* reserved */
    return SCFG_HDR_SIZE;
}

int stream_config_decode(const uint8_t *buf, size_t buf_sz, stream_config_t *cfg) {
    if (!buf || !cfg || buf_sz < SCFG_HDR_SIZE)
        return -1;
    if (r32le(buf) != (uint32_t)SCFG_MAGIC)
        return -1;

    memset(cfg, 0, sizeof(*cfg));
    cfg->video_codec = buf[4];
    cfg->audio_codec = buf[5];
    cfg->video_width = r16le(buf + 6);
    cfg->video_height = r16le(buf + 8);
    cfg->video_fps = buf[10];
    cfg->audio_channels = buf[11];
    cfg->video_bitrate_kbps = r32le(buf + 12);
    cfg->audio_bitrate_kbps = r32le(buf + 16);
    cfg->audio_sample_rate = r32le(buf + 20);
    cfg->transport_port = r16le(buf + 24);
    cfg->transport_proto = buf[26];
    cfg->flags = buf[27];
    return 0;
}

bool stream_config_equals(const stream_config_t *a, const stream_config_t *b) {
    if (!a || !b)
        return false;
    return memcmp(a, b, sizeof(*a)) == 0;
}

void stream_config_default(stream_config_t *cfg) {
    if (!cfg)
        return;
    memset(cfg, 0, sizeof(*cfg));
    cfg->video_codec = SCFG_VCODEC_H264;
    cfg->audio_codec = SCFG_ACODEC_OPUS;
    cfg->video_width = 1280;
    cfg->video_height = 720;
    cfg->video_fps = 30;
    cfg->audio_channels = 2;
    cfg->video_bitrate_kbps = 4000;
    cfg->audio_bitrate_kbps = 128;
    cfg->audio_sample_rate = 48000;
    cfg->transport_port = 5900;
    cfg->transport_proto = SCFG_PROTO_UDP;
    cfg->flags = 0;
}
