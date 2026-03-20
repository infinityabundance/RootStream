/*
 * m3u8_writer.h — M3U8 playlist manifest generator
 *
 * Generates HLS M3U8 manifests in two modes:
 *
 *   Live   — sliding window of @window_size most-recent segments;
 *            updates on each new segment.
 *
 *   VOD    — full list of all segments; written once at stream end.
 *
 * The output is written into a caller-supplied buffer (no heap alloc).
 *
 * Thread-safety: all functions are stateless and thread-safe.
 */

#ifndef ROOTSTREAM_M3U8_WRITER_H
#define ROOTSTREAM_M3U8_WRITER_H

#include <stdbool.h>
#include <stddef.h>

#include "hls_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A single HLS segment descriptor */
typedef struct {
    char filename[HLS_MAX_SEG_NAME]; /**< Segment file name (base) */
    double duration_s;               /**< Actual segment duration (s) */
    bool is_discontinuity;           /**< Insert #EXT-X-DISCONTINUITY */
} hls_segment_t;

/**
 * m3u8_write_live — generate a live sliding-window M3U8 into @buf
 *
 * @param segments      Full segment history array (oldest first)
 * @param n             Total number of segments
 * @param window_size   How many most-recent segments to include
 * @param target_dur_s  #EXT-X-TARGETDURATION value
 * @param media_seq     Starting #EXT-X-MEDIA-SEQUENCE value
 * @param buf           Output buffer
 * @param buf_sz        Size of @buf
 * @return              Bytes written (excl. NUL), or -1 if buf too small
 */
int m3u8_write_live(const hls_segment_t *segments, int n, int window_size, int target_dur_s,
                    int media_seq, char *buf, size_t buf_sz);

/**
 * m3u8_write_vod — generate a VOD (complete) M3U8 into @buf
 *
 * @param segments      All segment descriptors (oldest first)
 * @param n             Number of segments
 * @param target_dur_s  #EXT-X-TARGETDURATION value
 * @param buf           Output buffer
 * @param buf_sz        Size of @buf
 * @return              Bytes written (excl. NUL), or -1 if buf too small
 */
int m3u8_write_vod(const hls_segment_t *segments, int n, int target_dur_s, char *buf,
                   size_t buf_sz);

/**
 * m3u8_write_master — generate a master playlist for multi-bitrate HLS
 *
 * @param uris          Array of @n variant playlist URIs
 * @param bandwidths    Array of @n bandwidth values (bps)
 * @param n             Number of variants
 * @param width         Video width (0 = omit RESOLUTION)
 * @param height        Video height
 * @param buf           Output buffer
 * @param buf_sz        Size of @buf
 * @return              Bytes written, or -1
 */
int m3u8_write_master(const char **uris, const int *bandwidths, int n, int width, int height,
                      char *buf, size_t buf_sz);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_M3U8_WRITER_H */
