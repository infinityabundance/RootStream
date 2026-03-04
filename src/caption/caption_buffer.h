/*
 * caption_buffer.h — Caption timing ring-buffer
 *
 * Accepts caption events in arrival order (which may be out of PTS
 * order), keeps them sorted by PTS, and lets the renderer query which
 * events are currently active at a given playback timestamp.
 *
 * The buffer has a fixed capacity (CAPTION_BUFFER_CAPACITY entries).
 * Events whose end-time (PTS + duration) has passed the current
 * playback position are eligible for eviction by caption_buffer_expire().
 *
 * Thread-safety: NOT thread-safe; use external locking if needed.
 */

#ifndef ROOTSTREAM_CAPTION_BUFFER_H
#define ROOTSTREAM_CAPTION_BUFFER_H

#include "caption_event.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of events held in the buffer simultaneously */
#define CAPTION_BUFFER_CAPACITY 64

/** Opaque caption buffer */
typedef struct caption_buffer_s caption_buffer_t;

/**
 * caption_buffer_create — allocate empty caption buffer
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
caption_buffer_t *caption_buffer_create(void);

/**
 * caption_buffer_destroy — free caption buffer
 *
 * @param buf  Buffer to destroy
 */
void caption_buffer_destroy(caption_buffer_t *buf);

/**
 * caption_buffer_push — insert a caption event
 *
 * Events are inserted in PTS order (insertion sort).  If the buffer
 * is full the oldest event is silently discarded to make room.
 *
 * @param buf    Caption buffer
 * @param event  Event to insert (copied by value)
 * @return       0 on success, -1 on NULL args
 */
int caption_buffer_push(caption_buffer_t      *buf,
                         const caption_event_t *event);

/**
 * caption_buffer_query — fill @out with events active at @now_us
 *
 * @param buf        Caption buffer
 * @param now_us     Current playback timestamp
 * @param out        Array to receive active events
 * @param max_out    Capacity of @out array
 * @return           Number of active events written (>= 0)
 */
int caption_buffer_query(const caption_buffer_t *buf,
                          uint64_t                now_us,
                          caption_event_t        *out,
                          int                     max_out);

/**
 * caption_buffer_expire — remove events whose end-time < @now_us
 *
 * @param buf     Caption buffer
 * @param now_us  Current playback timestamp
 * @return        Number of events removed
 */
int caption_buffer_expire(caption_buffer_t *buf, uint64_t now_us);

/**
 * caption_buffer_count — return total events currently in buffer
 *
 * @param buf  Caption buffer
 * @return     Event count
 */
size_t caption_buffer_count(const caption_buffer_t *buf);

/**
 * caption_buffer_clear — remove all events
 *
 * @param buf  Caption buffer
 */
void caption_buffer_clear(caption_buffer_t *buf);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CAPTION_BUFFER_H */
