/*
 * config_serialiser.c — Versioned config envelope encode/decode
 */

#include "config_serialiser.h"

#include <string.h>

static void w16le(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}
static void w32le(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
static uint16_t r16le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t r32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int config_serialiser_encode(const stream_config_t *cfg,
                               uint8_t               *buf,
                               size_t                 buf_sz) {
    if (!cfg || !buf) return CSER_ERR_NULL;
    int total = config_serialiser_total_size();
    if (buf_sz < (size_t)total) return CSER_ERR_BUF_SMALL;

    w32le(buf, (uint32_t)CSER_ENVELOPE_MAGIC);
    w16le(buf + 4, (uint16_t)CSER_VERSION);
    w16le(buf + 6, (uint16_t)SCFG_HDR_SIZE);

    int n = stream_config_encode(cfg, buf + CSER_ENVELOPE_HDR_SIZE,
                                  buf_sz - CSER_ENVELOPE_HDR_SIZE);
    if (n < 0) return CSER_ERR_PAYLOAD;
    return total;
}

int config_serialiser_decode(const uint8_t  *buf,
                               size_t          buf_sz,
                               stream_config_t *cfg) {
    if (!buf || !cfg) return CSER_ERR_NULL;
    if (buf_sz < (size_t)CSER_ENVELOPE_HDR_SIZE) return CSER_ERR_BUF_SMALL;
    if (r32le(buf) != (uint32_t)CSER_ENVELOPE_MAGIC) return CSER_ERR_BAD_MAGIC;

    uint16_t ver = r16le(buf + 4);
    uint8_t major = (uint8_t)(ver >> 8);
    if (major != CSER_VERSION_MAJOR) return CSER_ERR_VERSION;

    uint16_t payload_len = r16le(buf + 6);
    if (buf_sz < (size_t)(CSER_ENVELOPE_HDR_SIZE + payload_len)) return CSER_ERR_BUF_SMALL;

    int rc = stream_config_decode(buf + CSER_ENVELOPE_HDR_SIZE,
                                   (size_t)payload_len, cfg);
    return (rc == 0) ? CSER_OK : CSER_ERR_PAYLOAD;
}
