/*
 * config_export.c — JSON export of stream configuration
 */

#include "config_export.h"

#include <stdio.h>
#include <inttypes.h>

const char *config_vcodec_name(uint8_t code) {
    switch (code) {
    case SCFG_VCODEC_RAW:  return "raw";
    case SCFG_VCODEC_H264: return "h264";
    case SCFG_VCODEC_H265: return "h265";
    case SCFG_VCODEC_AV1:  return "av1";
    case SCFG_VCODEC_VP9:  return "vp9";
    default:                return "unknown";
    }
}

const char *config_acodec_name(uint8_t code) {
    switch (code) {
    case SCFG_ACODEC_PCM:  return "pcm";
    case SCFG_ACODEC_OPUS: return "opus";
    case SCFG_ACODEC_AAC:  return "aac";
    case SCFG_ACODEC_FLAC: return "flac";
    default:                return "unknown";
    }
}

const char *config_proto_name(uint8_t code) {
    switch (code) {
    case SCFG_PROTO_UDP:  return "udp";
    case SCFG_PROTO_TCP:  return "tcp";
    case SCFG_PROTO_QUIC: return "quic";
    default:               return "unknown";
    }
}

int config_export_json(const stream_config_t *cfg,
                         char                  *buf,
                         size_t                 buf_sz) {
    if (!cfg || !buf || buf_sz == 0) return -1;

    int n = snprintf(buf, buf_sz,
        "{"
        "\"video_codec\":\"%s\","
        "\"video_width\":%" PRIu16 ","
        "\"video_height\":%" PRIu16 ","
        "\"video_fps\":%u,"
        "\"video_bitrate_kbps\":%" PRIu32 ","
        "\"audio_codec\":\"%s\","
        "\"audio_channels\":%u,"
        "\"audio_sample_rate\":%" PRIu32 ","
        "\"audio_bitrate_kbps\":%" PRIu32 ","
        "\"transport_proto\":\"%s\","
        "\"transport_port\":%" PRIu16 ","
        "\"encrypted\":%s,"
        "\"record\":%s,"
        "\"hw_encode\":%s"
        "}",
        config_vcodec_name(cfg->video_codec),
        cfg->video_width, cfg->video_height,
        (unsigned)cfg->video_fps,
        cfg->video_bitrate_kbps,
        config_acodec_name(cfg->audio_codec),
        (unsigned)cfg->audio_channels,
        cfg->audio_sample_rate, cfg->audio_bitrate_kbps,
        config_proto_name(cfg->transport_proto),
        cfg->transport_port,
        (cfg->flags & SCFG_FLAG_ENCRYPTED) ? "true" : "false",
        (cfg->flags & SCFG_FLAG_RECORD)    ? "true" : "false",
        (cfg->flags & SCFG_FLAG_HW_ENCODE) ? "true" : "false");

    if (n < 0 || (size_t)n >= buf_sz) return -1;
    return n;
}
