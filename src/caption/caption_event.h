/*
 * caption_event.h — Closed-caption event format
 *
 * A caption event represents one on-screen text segment: a UTF-8 string
 * with a presentation timestamp, duration, positioning, and style hints.
 *
 * Caption events are the atom of the closed-caption system; they flow
 * from a caption source (manual input, speech-recognition, or SRT file
 * parser) through the caption_buffer into the caption_renderer.
 *
 * Wire encoding (little-endian, used for ingest from remote sources)
 * ──────────────────────────────────────────────────────────────────
 *  Offset  Size  Field
 *   0      4     Magic   0x43415054 ('CAPT')
 *   4      8     PTS     presentation timestamp in µs
 *  12      4     Duration (µs)
 *  16      1     Flags   (CAPTION_FLAG_*)
 *  17      1     Row     (0–14, screen row)
 *  18      2     Text length (bytes)
 *  20      N     Text (UTF-8, <= CAPTION_MAX_TEXT_BYTES)
 */

#ifndef ROOTSTREAM_CAPTION_EVENT_H
#define ROOTSTREAM_CAPTION_EVENT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAPTION_MAGIC           0x43415054UL  /* 'CAPT' */
#define CAPTION_MAX_TEXT_BYTES  256
#define CAPTION_HDR_SIZE        20

/** Caption style flags */
#define CAPTION_FLAG_NONE       0x00
#define CAPTION_FLAG_BOLD       0x01
#define CAPTION_FLAG_ITALIC     0x02
#define CAPTION_FLAG_UNDERLINE  0x04
#define CAPTION_FLAG_BOTTOM     0x08  /**< Default: anchor at bottom */
#define CAPTION_FLAG_TOP        0x10  /**< Anchor at top of frame */

/** A single caption text segment */
typedef struct {
    uint64_t pts_us;            /**< Presentation timestamp (µs) */
    uint32_t duration_us;       /**< Display duration (µs) */
    uint8_t  flags;             /**< CAPTION_FLAG_* bitmask */
    uint8_t  row;               /**< Screen row 0–14 (0 = top) */
    uint16_t text_len;          /**< Byte length of text */
    char     text[CAPTION_MAX_TEXT_BYTES + 1];  /**< NUL-terminated UTF-8 */
} caption_event_t;

/**
 * caption_event_encode — serialise @event into @buf
 *
 * @param event   Event to encode
 * @param buf     Output buffer (must be >= CAPTION_HDR_SIZE + text_len)
 * @param buf_sz  Size of @buf
 * @return        Bytes written, or -1 on error / buffer too small
 */
int caption_event_encode(const caption_event_t *event,
                          uint8_t               *buf,
                          size_t                 buf_sz);

/**
 * caption_event_decode — parse @event from @buf
 *
 * @param buf     Input buffer
 * @param buf_sz  Valid bytes in @buf
 * @param event   Output event
 * @return        0 on success, -1 on parse error
 */
int caption_event_decode(const uint8_t   *buf,
                          size_t           buf_sz,
                          caption_event_t *event);

/**
 * caption_event_encoded_size — return serialised size for @event
 *
 * @param event  Caption event
 * @return       Byte count
 */
size_t caption_event_encoded_size(const caption_event_t *event);

/**
 * caption_event_is_active — return true if @event is visible at @now_us
 *
 * @param event   Caption event
 * @param now_us  Current playback timestamp in µs
 * @return        true if PTS <= now < PTS + duration
 */
bool caption_event_is_active(const caption_event_t *event, uint64_t now_us);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CAPTION_EVENT_H */
