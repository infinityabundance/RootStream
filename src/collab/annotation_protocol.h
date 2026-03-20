/*
 * annotation_protocol.h — Wire protocol for screen annotations
 *
 * Defines the binary framing used to exchange annotation events between
 * a host (presenter) and collaborating clients.  Each annotation event
 * is serialised into a compact packet and embedded in the DATA channel
 * of the RootStream transport.
 *
 * Packet layout (all integers little-endian)
 * ─────────────────────────────────────────
 *  Offset  Size  Field
 *  0       2     Magic  0x414E ('AN')
 *  2       1     Version (currently 1)
 *  3       1     Event type (annotation_event_type_t)
 *  4       4     Sequence number
 *  8       8     Timestamp (µs, monotonic)
 *  16      N     Payload (varies by event type)
 */

#ifndef ROOTSTREAM_ANNOTATION_PROTOCOL_H
#define ROOTSTREAM_ANNOTATION_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ANNOTATION_MAGIC 0x414EU /* 'AN' little-endian */
#define ANNOTATION_VERSION 1
#define ANNOTATION_HDR_SIZE 16 /* bytes */
#define ANNOTATION_MAX_TEXT 256

/** Annotation event types */
typedef enum {
    ANNOT_DRAW_BEGIN = 1,   /**< Pen/mouse down: start a new stroke */
    ANNOT_DRAW_POINT = 2,   /**< Intermediate stroke point */
    ANNOT_DRAW_END = 3,     /**< Pen/mouse up: finish stroke */
    ANNOT_ERASE = 4,        /**< Erase annotation in a circular region */
    ANNOT_CLEAR_ALL = 5,    /**< Clear all annotations */
    ANNOT_TEXT = 6,         /**< Place a text label */
    ANNOT_POINTER_MOVE = 7, /**< Remote cursor position update */
    ANNOT_POINTER_HIDE = 8, /**< Remote cursor hidden */
} annotation_event_type_t;

/** ARGB colour: 0xAARRGGBB */
typedef uint32_t annot_color_t;

/** Normalised 2-D coordinate: 0.0 = top/left, 1.0 = bottom/right */
typedef struct {
    float x;
    float y;
} annot_point_t;

/** Draw-begin payload */
typedef struct {
    annot_point_t pos;   /**< Starting position */
    annot_color_t color; /**< Stroke colour */
    float width;         /**< Stroke width in logical pixels */
    uint32_t stroke_id;  /**< Unique ID for this stroke */
} annot_draw_begin_t;

/** Draw-point payload */
typedef struct {
    annot_point_t pos;
    uint32_t stroke_id;
} annot_draw_point_t;

/** Draw-end payload */
typedef struct {
    uint32_t stroke_id;
} annot_draw_end_t;

/** Erase payload */
typedef struct {
    annot_point_t center;
    float radius; /**< Erase radius in normalised units */
} annot_erase_t;

/** Text annotation payload */
typedef struct {
    annot_point_t pos;
    annot_color_t color;
    float font_size;   /**< In logical pixels */
    uint16_t text_len; /**< Byte length of the UTF-8 text */
    char text[ANNOTATION_MAX_TEXT];
} annot_text_t;

/** Pointer-move payload */
typedef struct {
    annot_point_t pos;
    uint32_t peer_id; /**< Identifies the remote peer */
} annot_pointer_move_t;

/** Unified annotation event */
typedef struct {
    annotation_event_type_t type;
    uint32_t seq; /**< Monotonic sequence number */
    uint64_t timestamp_us;
    union {
        annot_draw_begin_t draw_begin;
        annot_draw_point_t draw_point;
        annot_draw_end_t draw_end;
        annot_erase_t erase;
        annot_text_t text;
        annot_pointer_move_t pointer_move;
    };
} annotation_event_t;

/**
 * annotation_encode — serialise @event into @buf
 *
 * @param event   Event to serialise
 * @param buf     Output buffer
 * @param buf_sz  Size of @buf in bytes
 * @return        Number of bytes written, or -1 if buf_sz too small
 */
int annotation_encode(const annotation_event_t *event, uint8_t *buf, size_t buf_sz);

/**
 * annotation_decode — deserialise @event from @buf
 *
 * @param buf     Input buffer
 * @param buf_sz  Valid bytes in @buf
 * @param event   Output event
 * @return        0 on success, -1 on parse error
 */
int annotation_decode(const uint8_t *buf, size_t buf_sz, annotation_event_t *event);

/**
 * annotation_encoded_size — compute the serialised size of @event
 *
 * @param event  Event
 * @return       Byte count needed by annotation_encode()
 */
size_t annotation_encoded_size(const annotation_event_t *event);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_ANNOTATION_PROTOCOL_H */
