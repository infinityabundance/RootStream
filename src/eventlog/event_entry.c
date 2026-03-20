/*
 * event_entry.c — Event log entry encode / decode
 */

#include "event_entry.h"

#include <string.h>

static void w16le(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}
static void w64le(uint8_t *p, uint64_t v) {
    for (int i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (i * 8));
}
static uint16_t r16le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint64_t r64le(const uint8_t *p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= ((uint64_t)p[i] << (i * 8));
    return v;
}

int event_entry_encode(const event_entry_t *e, uint8_t *buf, size_t buf_sz) {
    if (!e || !buf)
        return -1;
    size_t msglen = strnlen(e->msg, EVENT_MSG_MAX - 1) + 1; /* include NUL */
    size_t total = EVENT_ENTRY_HDR_SIZE + msglen;
    if (buf_sz < total)
        return -1;

    w64le(buf + 0, e->timestamp_us);
    buf[8] = (uint8_t)e->level;
    buf[9] = 0;
    w16le(buf + 10, e->event_type);
    w16le(buf + 12, (uint16_t)msglen);
    w16le(buf + 14, 0);
    memcpy(buf + EVENT_ENTRY_HDR_SIZE, e->msg, msglen);
    return (int)total;
}

int event_entry_decode(const uint8_t *buf, size_t buf_sz, event_entry_t *e) {
    if (!buf || !e || buf_sz < EVENT_ENTRY_HDR_SIZE)
        return -1;

    uint16_t msglen = r16le(buf + 12);
    if (msglen == 0 || msglen > EVENT_MSG_MAX)
        return -1;
    if (buf_sz < (size_t)(EVENT_ENTRY_HDR_SIZE + msglen))
        return -1;

    memset(e, 0, sizeof(*e));
    e->timestamp_us = r64le(buf + 0);
    e->level = (event_level_t)buf[8];
    e->event_type = r16le(buf + 10);
    memcpy(e->msg, buf + EVENT_ENTRY_HDR_SIZE, msglen);
    e->msg[EVENT_MSG_MAX - 1] = '\0'; /* ensure termination */
    return 0;
}

const char *event_level_name(event_level_t l) {
    switch (l) {
        case EVENT_LEVEL_DEBUG:
            return "DEBUG";
        case EVENT_LEVEL_INFO:
            return "INFO";
        case EVENT_LEVEL_WARN:
            return "WARN";
        case EVENT_LEVEL_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}
