/*
 * stream_config.h — Stream configuration record
 *
 * Captures the complete set of runtime-configurable parameters for
 * one streaming session: video codec, resolution, target bitrate,
 * audio codec, channel layout, and transport settings.
 *
 * Wire encoding (little-endian)
 * ─────────────────────────────
 *  Offset  Size  Field
 *   0      4     Magic         0x53434647 ('SCFG')
 *   4      1     video_codec   — SCFG_CODEC_* enum
 *   5      1     audio_codec   — SCFG_ACODEC_* enum
 *   6      2     video_width
 *   8      2     video_height
 *  10      1     video_fps
 *  11      1     audio_channels
 *  12      4     video_bitrate_kbps
 *  16      4     audio_bitrate_kbps
 *  20      4     audio_sample_rate
 *  24      2     transport_port
 *  26      1     transport_proto — SCFG_PROTO_* enum
 *  27      1     flags
 *  28      4     reserved
 *  32      N     End of fixed header (32 bytes total)
 */

#ifndef ROOTSTREAM_STREAM_CONFIG_H
#define ROOTSTREAM_STREAM_CONFIG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCFG_MAGIC         0x53434647UL  /* 'SCFG' */
#define SCFG_HDR_SIZE      32

/* ── Video codec constants ──────────────────────────────────────── */
#define SCFG_VCODEC_RAW    0
#define SCFG_VCODEC_H264   1
#define SCFG_VCODEC_H265   2
#define SCFG_VCODEC_AV1    3
#define SCFG_VCODEC_VP9    4

/* ── Audio codec constants ──────────────────────────────────────── */
#define SCFG_ACODEC_PCM    0
#define SCFG_ACODEC_OPUS   1
#define SCFG_ACODEC_AAC    2
#define SCFG_ACODEC_FLAC   3

/* ── Transport protocol constants ───────────────────────────────── */
#define SCFG_PROTO_UDP     0
#define SCFG_PROTO_TCP     1
#define SCFG_PROTO_QUIC    2

/* ── Flags ──────────────────────────────────────────────────────── */
#define SCFG_FLAG_ENCRYPTED  0x01  /**< Enable transport encryption */
#define SCFG_FLAG_RECORD     0x02  /**< Enable local recording */
#define SCFG_FLAG_HW_ENCODE  0x04  /**< Prefer hardware encoder */

/** Stream configuration record */
typedef struct {
    uint8_t  video_codec;
    uint8_t  audio_codec;
    uint16_t video_width;
    uint16_t video_height;
    uint8_t  video_fps;
    uint8_t  audio_channels;
    uint32_t video_bitrate_kbps;
    uint32_t audio_bitrate_kbps;
    uint32_t audio_sample_rate;
    uint16_t transport_port;
    uint8_t  transport_proto;
    uint8_t  flags;
} stream_config_t;

/**
 * stream_config_encode — serialise @cfg into @buf
 *
 * @param cfg     Config to encode
 * @param buf     Output buffer (>= SCFG_HDR_SIZE bytes)
 * @param buf_sz  Buffer size
 * @return        Bytes written (SCFG_HDR_SIZE), or -1 on error
 */
int stream_config_encode(const stream_config_t *cfg,
                           uint8_t               *buf,
                           size_t                 buf_sz);

/**
 * stream_config_decode — parse @cfg from @buf
 *
 * @param buf     Input buffer
 * @param buf_sz  Valid bytes in @buf
 * @param cfg     Output config
 * @return        0 on success, -1 on error
 */
int stream_config_decode(const uint8_t  *buf,
                           size_t          buf_sz,
                           stream_config_t *cfg);

/**
 * stream_config_equals — return true if two configs are identical
 *
 * @param a  First config
 * @param b  Second config
 * @return   true if equal
 */
bool stream_config_equals(const stream_config_t *a, const stream_config_t *b);

/**
 * stream_config_default — populate @cfg with sensible defaults
 *
 *   1280×720 30fps H.264 @ 4000 kbps / Opus 2ch 48 kHz @ 128 kbps / UDP:5900
 *
 * @param cfg  Config to populate
 */
void stream_config_default(stream_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_STREAM_CONFIG_H */
