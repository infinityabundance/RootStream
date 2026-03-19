/*
 * hls_config.h — HLS output configuration constants
 *
 * Central location for all HLS-related compile-time defaults.
 * Callers may override via hls_segmenter_config_t at runtime.
 */

#ifndef ROOTSTREAM_HLS_CONFIG_H
#define ROOTSTREAM_HLS_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/** Default target segment duration in seconds */
#define HLS_DEFAULT_SEGMENT_DURATION_S 6

/** Default number of segments to keep in a live sliding-window playlist */
#define HLS_DEFAULT_WINDOW_SEGMENTS 5

/** Maximum path length for HLS output directory */
#define HLS_MAX_PATH 512

/** Maximum segment filename length (base name, not full path) */
#define HLS_MAX_SEG_NAME 64

/** Maximum number of bitrate variants in a multi-bitrate ladder */
#define HLS_MAX_VARIANTS 4

/** MPEG-TS packet size in bytes */
#define HLS_TS_PACKET_SZ 188

/** Sync byte for MPEG-TS packets */
#define HLS_TS_SYNC_BYTE 0x47

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_HLS_CONFIG_H */
