/*
 * event_ring.h — Fixed-capacity analytics event ring buffer
 *
 * A lock-free-friendly single-producer/single-consumer ring buffer
 * for analytics events.  For multi-producer use, wrap push() calls
 * with an external mutex.
 *
 * When the ring is full, the oldest event is silently overwritten
 * ("loss-less head-drop" policy).
 */

#ifndef ROOTSTREAM_EVENT_RING_H
#define ROOTSTREAM_EVENT_RING_H

#include <stdbool.h>
#include <stddef.h>

#include "analytics_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Ring buffer capacity (number of events) */
#define EVENT_RING_CAPACITY 1024

/** Opaque ring buffer handle */
typedef struct event_ring_s event_ring_t;

/**
 * event_ring_create — allocate ring buffer
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
event_ring_t *event_ring_create(void);

/**
 * event_ring_destroy — free ring buffer
 *
 * @param r  Ring to destroy
 */
void event_ring_destroy(event_ring_t *r);

/**
 * event_ring_push — enqueue @event (overwrites oldest if full)
 *
 * @param r      Ring
 * @param event  Event to enqueue (copied by value)
 * @return       0 on success, -1 on NULL args
 */
int event_ring_push(event_ring_t *r, const analytics_event_t *event);

/**
 * event_ring_pop — dequeue oldest event
 *
 * @param r    Ring
 * @param out  Output event
 * @return     0 on success, -1 if empty
 */
int event_ring_pop(event_ring_t *r, analytics_event_t *out);

/**
 * event_ring_peek — copy oldest event without removing it
 *
 * @param r    Ring
 * @param out  Output event
 * @return     0 on success, -1 if empty
 */
int event_ring_peek(const event_ring_t *r, analytics_event_t *out);

/**
 * event_ring_count — number of events currently in ring
 *
 * @param r  Ring
 * @return   Event count [0, EVENT_RING_CAPACITY]
 */
size_t event_ring_count(const event_ring_t *r);

/**
 * event_ring_is_empty — return true if ring has no events
 *
 * @param r  Ring
 */
bool event_ring_is_empty(const event_ring_t *r);

/**
 * event_ring_clear — discard all events
 *
 * @param r  Ring
 */
void event_ring_clear(event_ring_t *r);

/**
 * event_ring_drain — move up to @max events into @out array
 *
 * @param r    Ring
 * @param out  Output array
 * @param max  Maximum events to drain
 * @return     Number of events placed in @out
 */
size_t event_ring_drain(event_ring_t *r, analytics_event_t *out, size_t max);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_EVENT_RING_H */
