/*
 * annotation_protocol.c — Annotation wire protocol serialisation
 */

#include "annotation_protocol.h"

#include <string.h>
#include <stdio.h>

/* ── Helpers ─────────────────────────────────────────────────────── */

static void write_u16_le(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
}

static void write_u32_le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void write_u64_le(uint8_t *p, uint64_t v) {
    for (int i = 0; i < 8; i++) {
        p[i] = (uint8_t)(v >> (i * 8));
    }
}

static void write_f32(uint8_t *p, float v) {
    memcpy(p, &v, 4);
}

static uint16_t read_u16_le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_u32_le(const uint8_t *p) {
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static uint64_t read_u64_le(const uint8_t *p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= ((uint64_t)p[i] << (i * 8));
    return v;
}

static float read_f32(const uint8_t *p) {
    float v;
    memcpy(&v, p, 4);
    return v;
}

/* ── Payload sizes ────────────────────────────────────────────────── */

static size_t payload_size(const annotation_event_t *e) {
    switch (e->type) {
    case ANNOT_DRAW_BEGIN:
        return 2*4 + 4 + 4 + 4;  /* pos(2×f32) + color + width + stroke_id */
    case ANNOT_DRAW_POINT:
        return 2*4 + 4;           /* pos + stroke_id */
    case ANNOT_DRAW_END:
        return 4;                  /* stroke_id */
    case ANNOT_ERASE:
        return 2*4 + 4;           /* center + radius */
    case ANNOT_CLEAR_ALL:
        return 0;
    case ANNOT_TEXT:
        return 2*4 + 4 + 4 + 2 + (size_t)e->text.text_len;
    case ANNOT_POINTER_MOVE:
        return 2*4 + 4;           /* pos + peer_id */
    case ANNOT_POINTER_HIDE:
        return 0;
    default:
        return 0;
    }
}

size_t annotation_encoded_size(const annotation_event_t *event) {
    if (!event) return 0;
    return ANNOTATION_HDR_SIZE + payload_size(event);
}

/* ── Encode ───────────────────────────────────────────────────────── */

int annotation_encode(const annotation_event_t *event,
                      uint8_t                  *buf,
                      size_t                    buf_sz) {
    if (!event || !buf) return -1;

    size_t needed = annotation_encoded_size(event);
    if (buf_sz < needed) return -1;

    /* Header */
    write_u16_le(buf + 0,  (uint16_t)ANNOTATION_MAGIC);
    buf[2] = ANNOTATION_VERSION;
    buf[3] = (uint8_t)event->type;
    write_u32_le(buf + 4,  event->seq);
    write_u64_le(buf + 8,  event->timestamp_us);

    uint8_t *p = buf + ANNOTATION_HDR_SIZE;

    switch (event->type) {
    case ANNOT_DRAW_BEGIN:
        write_f32(p, event->draw_begin.pos.x);    p += 4;
        write_f32(p, event->draw_begin.pos.y);    p += 4;
        write_u32_le(p, event->draw_begin.color); p += 4;
        write_f32(p, event->draw_begin.width);    p += 4;
        write_u32_le(p, event->draw_begin.stroke_id);
        break;
    case ANNOT_DRAW_POINT:
        write_f32(p, event->draw_point.pos.x);    p += 4;
        write_f32(p, event->draw_point.pos.y);    p += 4;
        write_u32_le(p, event->draw_point.stroke_id);
        break;
    case ANNOT_DRAW_END:
        write_u32_le(p, event->draw_end.stroke_id);
        break;
    case ANNOT_ERASE:
        write_f32(p, event->erase.center.x);  p += 4;
        write_f32(p, event->erase.center.y);  p += 4;
        write_f32(p, event->erase.radius);
        break;
    case ANNOT_CLEAR_ALL:
        break;
    case ANNOT_TEXT: {
        write_f32(p, event->text.pos.x);          p += 4;
        write_f32(p, event->text.pos.y);          p += 4;
        write_u32_le(p, event->text.color);        p += 4;
        write_f32(p, event->text.font_size);       p += 4;
        uint16_t tlen = event->text.text_len;
        write_u16_le(p, tlen);                     p += 2;
        memcpy(p, event->text.text, tlen);
        break;
    }
    case ANNOT_POINTER_MOVE:
        write_f32(p, event->pointer_move.pos.x);       p += 4;
        write_f32(p, event->pointer_move.pos.y);       p += 4;
        write_u32_le(p, event->pointer_move.peer_id);
        break;
    case ANNOT_POINTER_HIDE:
        break;
    default:
        return -1;
    }

    return (int)needed;
}

/* ── Decode ───────────────────────────────────────────────────────── */

int annotation_decode(const uint8_t      *buf,
                      size_t              buf_sz,
                      annotation_event_t *event) {
    if (!buf || !event || buf_sz < ANNOTATION_HDR_SIZE) return -1;

    uint16_t magic = read_u16_le(buf);
    if (magic != (uint16_t)ANNOTATION_MAGIC) return -1;
    if (buf[2] != ANNOTATION_VERSION) return -1;

    memset(event, 0, sizeof(*event));
    event->type         = (annotation_event_type_t)buf[3];
    event->seq          = read_u32_le(buf + 4);
    event->timestamp_us = read_u64_le(buf + 8);

    const uint8_t *p = buf + ANNOTATION_HDR_SIZE;
    size_t remaining  = buf_sz - ANNOTATION_HDR_SIZE;

    switch (event->type) {
    case ANNOT_DRAW_BEGIN:
        if (remaining < 20) return -1;
        event->draw_begin.pos.x    = read_f32(p); p += 4;
        event->draw_begin.pos.y    = read_f32(p); p += 4;
        event->draw_begin.color    = read_u32_le(p); p += 4;
        event->draw_begin.width    = read_f32(p); p += 4;
        event->draw_begin.stroke_id= read_u32_le(p);
        break;
    case ANNOT_DRAW_POINT:
        if (remaining < 12) return -1;
        event->draw_point.pos.x     = read_f32(p); p += 4;
        event->draw_point.pos.y     = read_f32(p); p += 4;
        event->draw_point.stroke_id = read_u32_le(p);
        break;
    case ANNOT_DRAW_END:
        if (remaining < 4) return -1;
        event->draw_end.stroke_id = read_u32_le(p);
        break;
    case ANNOT_ERASE:
        if (remaining < 12) return -1;
        event->erase.center.x = read_f32(p); p += 4;
        event->erase.center.y = read_f32(p); p += 4;
        event->erase.radius   = read_f32(p);
        break;
    case ANNOT_CLEAR_ALL:
        break;
    case ANNOT_TEXT:
        if (remaining < 18) return -1;
        event->text.pos.x     = read_f32(p); p += 4;
        event->text.pos.y     = read_f32(p); p += 4;
        event->text.color     = read_u32_le(p); p += 4;
        event->text.font_size = read_f32(p); p += 4;
        event->text.text_len  = read_u16_le(p); p += 2;
        if (event->text.text_len > ANNOTATION_MAX_TEXT) return -1;
        if (remaining - 18 < event->text.text_len) return -1;
        memcpy(event->text.text, p, event->text.text_len);
        break;
    case ANNOT_POINTER_MOVE:
        if (remaining < 12) return -1;
        event->pointer_move.pos.x   = read_f32(p); p += 4;
        event->pointer_move.pos.y   = read_f32(p); p += 4;
        event->pointer_move.peer_id = read_u32_le(p);
        break;
    case ANNOT_POINTER_HIDE:
        break;
    default:
        return -1;
    }

    return 0;
}
