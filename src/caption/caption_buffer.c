/*
 * caption_buffer.c — Caption timing ring-buffer implementation
 */

#include "caption_buffer.h"

#include <stdlib.h>
#include <string.h>

struct caption_buffer_s {
    caption_event_t events[CAPTION_BUFFER_CAPACITY];
    int count;
};

caption_buffer_t *caption_buffer_create(void) {
    return calloc(1, sizeof(caption_buffer_t));
}

void caption_buffer_destroy(caption_buffer_t *buf) {
    free(buf);
}

void caption_buffer_clear(caption_buffer_t *buf) {
    if (!buf)
        return;
    buf->count = 0;
}

size_t caption_buffer_count(const caption_buffer_t *buf) {
    return buf ? (size_t)buf->count : 0;
}

int caption_buffer_push(caption_buffer_t *buf, const caption_event_t *event) {
    if (!buf || !event)
        return -1;

    /* If full, drop the oldest (index 0) */
    if (buf->count >= CAPTION_BUFFER_CAPACITY) {
        memmove(&buf->events[0], &buf->events[1],
                (size_t)(buf->count - 1) * sizeof(caption_event_t));
        buf->count--;
    }

    /* Insertion sort by pts_us */
    int pos = buf->count;
    while (pos > 0 && buf->events[pos - 1].pts_us > event->pts_us) {
        buf->events[pos] = buf->events[pos - 1];
        pos--;
    }
    buf->events[pos] = *event;
    buf->count++;
    return 0;
}

int caption_buffer_query(const caption_buffer_t *buf, uint64_t now_us, caption_event_t *out,
                         int max_out) {
    if (!buf || !out || max_out <= 0)
        return 0;

    int n = 0;
    for (int i = 0; i < buf->count && n < max_out; i++) {
        if (caption_event_is_active(&buf->events[i], now_us)) {
            out[n++] = buf->events[i];
        }
    }
    return n;
}

int caption_buffer_expire(caption_buffer_t *buf, uint64_t now_us) {
    if (!buf)
        return 0;

    int removed = 0;
    int out = 0;
    for (int i = 0; i < buf->count; i++) {
        uint64_t end = buf->events[i].pts_us + (uint64_t)buf->events[i].duration_us;
        if (end <= now_us) {
            removed++;
        } else {
            buf->events[out++] = buf->events[i];
        }
    }
    buf->count = out;
    return removed;
}
