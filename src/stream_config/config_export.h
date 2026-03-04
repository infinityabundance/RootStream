/*
 * config_export.h — JSON serialisation of stream_config_t
 *
 * Renders a stream_config_t as a compact JSON object into a caller-
 * supplied buffer.  No heap allocation.
 *
 * Thread-safety: stateless, thread-safe.
 */

#ifndef ROOTSTREAM_CONFIG_EXPORT_H
#define ROOTSTREAM_CONFIG_EXPORT_H

#include "stream_config.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * config_export_json — render @cfg as JSON into @buf
 *
 * Example output:
 *   {"video_codec":"h264","video_width":1280,"video_height":720,
 *    "video_fps":30,"video_bitrate_kbps":4000,
 *    "audio_codec":"opus","audio_channels":2,
 *    "audio_sample_rate":48000,"audio_bitrate_kbps":128,
 *    "transport_proto":"udp","transport_port":5900,
 *    "encrypted":false,"record":false,"hw_encode":false}
 *
 * @param cfg     Config to render
 * @param buf     Output buffer
 * @param buf_sz  Buffer size
 * @return        Bytes written (excl. NUL), or -1 if buf too small
 */
int config_export_json(const stream_config_t *cfg,
                         char                  *buf,
                         size_t                 buf_sz);

/**
 * config_vcodec_name — return codec name string for @code
 *
 * @param code  SCFG_VCODEC_* constant
 * @return      Static string ("raw", "h264", "h265", "av1", "vp9", "unknown")
 */
const char *config_vcodec_name(uint8_t code);

/**
 * config_acodec_name — return audio codec name string for @code
 *
 * @param code  SCFG_ACODEC_* constant
 * @return      Static string ("pcm", "opus", "aac", "flac", "unknown")
 */
const char *config_acodec_name(uint8_t code);

/**
 * config_proto_name — return transport protocol name for @code
 *
 * @param code  SCFG_PROTO_* constant
 * @return      Static string ("udp", "tcp", "quic", "unknown")
 */
const char *config_proto_name(uint8_t code);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CONFIG_EXPORT_H */
