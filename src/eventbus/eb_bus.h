/*
 * eb_bus.h — Event Bus: pub/sub dispatcher
 *
 * Supports up to EB_MAX_SUBSCRIBERS subscriptions.  Each subscription
 * binds a callback to a specific event type_id.  Publishing an event
 * invokes all matching callbacks synchronously in subscription order.
 *
 * An EB_TYPE_ANY wildcard can be used to receive all events.
 * Subscriptions are identified by an opaque handle returned by
 * eb_bus_subscribe() and used to unsubscribe.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_EB_BUS_H
#define ROOTSTREAM_EB_BUS_H

#include <stdint.h>

#include "eb_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EB_MAX_SUBSCRIBERS 16
#define EB_TYPE_ANY UINT32_MAX /**< Wildcard: match all event types */

/** Subscription callback */
typedef void (*eb_callback_t)(const eb_event_t *event, void *user);

/** Opaque subscription handle */
typedef int eb_handle_t;
#define EB_INVALID_HANDLE (-1)

/** Opaque event bus */
typedef struct eb_bus_s eb_bus_t;

/**
 * eb_bus_create — allocate event bus
 *
 * @return Non-NULL handle, or NULL on OOM
 */
eb_bus_t *eb_bus_create(void);

/**
 * eb_bus_destroy — free event bus
 */
void eb_bus_destroy(eb_bus_t *b);

/**
 * eb_bus_subscribe — register a callback for a specific event type
 *
 * @param b       Bus
 * @param type_id Event type to subscribe to (or EB_TYPE_ANY)
 * @param cb      Callback function
 * @param user    Opaque user pointer passed to callback
 * @return        Non-negative handle, or EB_INVALID_HANDLE on full/invalid
 */
eb_handle_t eb_bus_subscribe(eb_bus_t *b, eb_type_t type_id, eb_callback_t cb, void *user);

/**
 * eb_bus_unsubscribe — remove a subscription
 *
 * @param b   Bus
 * @param h   Handle from eb_bus_subscribe()
 * @return    0 on success, -1 if handle not found
 */
int eb_bus_unsubscribe(eb_bus_t *b, eb_handle_t h);

/**
 * eb_bus_publish — dispatch an event to all matching subscribers
 *
 * @param b  Bus
 * @param e  Event to dispatch (caller owns)
 * @return   Number of subscribers invoked (0 = no subscribers / dropped)
 */
int eb_bus_publish(eb_bus_t *b, const eb_event_t *e);

/**
 * eb_bus_subscriber_count — current number of active subscriptions
 */
int eb_bus_subscriber_count(const eb_bus_t *b);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_EB_BUS_H */
