/*
 * stream_metadata.h — Stream metadata record format
 *
 * A stream_metadata_t is a snapshot of the current stream's descriptive
 * properties: title, description, tags, start time, and codec parameters.
 *
 * Wire encoding (little-endian)
 * ──────────────────────────────
 *  Offset  Size  Field
 *   0      4     Magic         0x4D455441 ('META')
 *   4      8     start_us      — stream start time (µs since epoch)
 *  12      4     duration_us   — running duration; 0 if live
 *  16      2     video_width
 *  18      2     video_height
 *  20      1     video_fps
 *  21      1     flags
 *  22      2     title_len
 *  24      N     title (UTF-8, <= METADATA_MAX_TITLE)
 *  ...     2     desc_len
 *  ...     N     description
 *  ...     2     tags_len
 *  ...     N     tags (comma-separated)
 */

#ifndef ROOTSTREAM_STREAM_METADATA_H
#define ROOTSTREAM_STREAM_METADATA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define METADATA_MAGIC          0x4D455441UL  /* 'META' */
#define METADATA_FIXED_HDR_SZ   22
#define METADATA_MAX_TITLE      256
#define METADATA_MAX_DESC       1024
#define METADATA_MAX_TAGS       512

/** Metadata flags */
#define METADATA_FLAG_LIVE       0x01  /**< Live stream (not VOD) */
#define METADATA_FLAG_ENCRYPTED  0x02  /**< Stream uses encryption */
#define METADATA_FLAG_PUBLIC     0x04  /**< Stream is publicly visible */

/** Stream metadata record */
typedef struct {
    uint64_t start_us;
    uint32_t duration_us;
    uint16_t video_width;
    uint16_t video_height;
    uint8_t  video_fps;
    uint8_t  flags;
    char     title[METADATA_MAX_TITLE + 1];
    char     description[METADATA_MAX_DESC + 1];
    char     tags[METADATA_MAX_TAGS + 1];
} stream_metadata_t;

/**
 * stream_metadata_encode — serialise @meta into @buf
 *
 * @param meta    Metadata to encode
 * @param buf     Output buffer
 * @param buf_sz  Buffer size
 * @return        Bytes written, or -1 on error
 */
int stream_metadata_encode(const stream_metadata_t *meta,
                             uint8_t                 *buf,
                             size_t                   buf_sz);

/**
 * stream_metadata_decode — parse @meta from @buf
 *
 * @param buf     Input buffer
 * @param buf_sz  Valid bytes in @buf
 * @param meta    Output metadata
 * @return        0 on success, -1 on error
 */
int stream_metadata_decode(const uint8_t    *buf,
                             size_t            buf_sz,
                             stream_metadata_t *meta);

/**
 * stream_metadata_is_live — return true if METADATA_FLAG_LIVE is set
 *
 * @param meta  Metadata
 * @return      true if live
 */
bool stream_metadata_is_live(const stream_metadata_t *meta);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_STREAM_METADATA_H */
