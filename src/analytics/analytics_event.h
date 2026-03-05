/*
 * analytics_event.h — Structured analytics event types
 *
 * Defines the event taxonomy used by the viewer analytics pipeline.
 * Events are small fixed-size structs designed to be stored in a
 * ring buffer and flushed to JSON or CSV.
 *
 * Wire encoding (little-endian, used for binary log files)
 * ─────────────────────────────────────────────────────────
 *  Offset  Size  Field
 *   0      4     Magic   0x414E4C59 ('ANLY')
 *   4      8     timestamp_us (µs since Unix epoch)
 *  12      1     event_type (analytics_event_type_t)
 *  13      1     flags
 *  14      2     payload_len (bytes; 0 = no payload)
 *  16      8     session_id
 *  24      8     value  (type-specific numeric value)
 *  32      N     payload (UTF-8, <= ANALYTICS_MAX_PAYLOAD bytes)
 */

#ifndef ROOTSTREAM_ANALYTICS_EVENT_H
#define ROOTSTREAM_ANALYTICS_EVENT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ANALYTICS_MAGIC         0x414E4C59UL  /* 'ANLY' */
#define ANALYTICS_HDR_SIZE      32
#define ANALYTICS_MAX_PAYLOAD   128

/** Analytics event types */
typedef enum {
    ANALYTICS_VIEWER_JOIN    = 0x01,  /**< Viewer connected */
    ANALYTICS_VIEWER_LEAVE   = 0x02,  /**< Viewer disconnected */
    ANALYTICS_BITRATE_CHANGE = 0x03,  /**< Encoder bitrate changed; value=kbps */
    ANALYTICS_FRAME_DROP     = 0x04,  /**< Frame dropped; value=drop_count */
    ANALYTICS_QUALITY_ALERT  = 0x05,  /**< Quality below threshold */
    ANALYTICS_SCENE_CHANGE   = 0x06,  /**< Scene cut detected */
    ANALYTICS_STREAM_START   = 0x07,  /**< Stream started */
    ANALYTICS_STREAM_STOP    = 0x08,  /**< Stream stopped */
    ANALYTICS_LATENCY_SAMPLE = 0x09,  /**< Latency sample; value=µs */
} analytics_event_type_t;

/** Single analytics event */
typedef struct {
    uint64_t               timestamp_us;
    analytics_event_type_t type;
    uint8_t                flags;
    uint16_t               payload_len;
    uint64_t               session_id;
    uint64_t               value;
    char                   payload[ANALYTICS_MAX_PAYLOAD + 1];
} analytics_event_t;

/**
 * analytics_event_encode — serialise @event into @buf
 *
 * @param event   Event to encode
 * @param buf     Output buffer (>= ANALYTICS_HDR_SIZE + payload_len)
 * @param buf_sz  Size of @buf
 * @return        Bytes written, or -1 on error
 */
int analytics_event_encode(const analytics_event_t *event,
                             uint8_t                 *buf,
                             size_t                   buf_sz);

/**
 * analytics_event_decode — parse event from @buf
 *
 * @param buf     Input buffer
 * @param buf_sz  Valid bytes
 * @param event   Output event
 * @return        0 on success, -1 on error
 */
int analytics_event_decode(const uint8_t      *buf,
                             size_t              buf_sz,
                             analytics_event_t  *event);

/**
 * analytics_event_encoded_size — return serialised byte count for @event
 */
size_t analytics_event_encoded_size(const analytics_event_t *event);

/**
 * analytics_event_type_name — return human-readable type name
 *
 * @param type  Event type
 * @return      Static string (never NULL)
 */
const char *analytics_event_type_name(analytics_event_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_ANALYTICS_EVENT_H */
