/*
 * schedule_entry.c — Stream schedule entry serialisation
 */

#include "schedule_entry.h"

#include <string.h>

/* ── Little-endian helpers ─────────────────────────────────────── */

static void w16le(uint8_t *p, uint16_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
}
static void w32le(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
static void w64le(uint8_t *p, uint64_t v) {
    for (int i=0;i<8;i++) p[i]=(uint8_t)(v>>(i*8));
}
static uint16_t r16le(const uint8_t *p) {
    return (uint16_t)p[0]|((uint16_t)p[1]<<8);
}
static uint32_t r32le(const uint8_t *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint64_t r64le(const uint8_t *p) {
    uint64_t v=0;
    for(int i=0;i<8;i++) v|=((uint64_t)p[i]<<(i*8));
    return v;
}

/* ── Public API ────────────────────────────────────────────────── */

size_t schedule_entry_encoded_size(const schedule_entry_t *entry) {
    if (!entry) return 0;
    return SCHEDULE_HDR_SIZE + (size_t)entry->title_len;
}

bool schedule_entry_is_enabled(const schedule_entry_t *entry) {
    return entry ? !!(entry->flags & SCHED_FLAG_ENABLED) : false;
}

int schedule_entry_encode(const schedule_entry_t *entry,
                           uint8_t                *buf,
                           size_t                  buf_sz) {
    if (!entry || !buf) return -1;
    size_t needed = schedule_entry_encoded_size(entry);
    if (buf_sz < needed) return -1;

    w32le(buf +  0, (uint32_t)SCHEDULE_MAGIC);
    w64le(buf +  4, entry->start_us);
    w32le(buf + 12, entry->duration_us);
    buf[16] = (uint8_t)entry->source_type;
    buf[17] = entry->flags;
    w16le(buf + 18, entry->title_len);
    if (entry->title_len > 0)
        memcpy(buf + SCHEDULE_HDR_SIZE, entry->title, entry->title_len);
    return (int)needed;
}

int schedule_entry_decode(const uint8_t    *buf,
                           size_t            buf_sz,
                           schedule_entry_t *entry) {
    if (!buf || !entry || buf_sz < SCHEDULE_HDR_SIZE) return -1;
    if (r32le(buf) != (uint32_t)SCHEDULE_MAGIC) return -1;

    memset(entry, 0, sizeof(*entry));
    entry->start_us     = r64le(buf + 4);
    entry->duration_us  = r32le(buf + 12);
    entry->source_type  = (schedule_source_t)buf[16];
    entry->flags        = buf[17];
    entry->title_len    = r16le(buf + 18);

    if (entry->title_len > SCHEDULE_MAX_TITLE) return -1;
    if (buf_sz < SCHEDULE_HDR_SIZE + (size_t)entry->title_len) return -1;
    if (entry->title_len > 0)
        memcpy(entry->title, buf + SCHEDULE_HDR_SIZE, entry->title_len);
    entry->title[entry->title_len] = '\0';
    return 0;
}
