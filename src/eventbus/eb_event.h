/*
 * eb_event.h — Event Bus: event descriptor
 *
 * An event carries a numeric type identifier, an opaque payload
 * pointer (owned by the caller — not copied or freed by the bus),
 * the payload length in bytes, and the wall-clock timestamp (µs)
 * at which the event was created.
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_EB_EVENT_H
#define ROOTSTREAM_EB_EVENT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Event type identifiers — extend as needed */
typedef uint32_t eb_type_t;

/** Event descriptor */
typedef struct {
    eb_type_t type_id;     /**< Numeric event type */
    void *payload;         /**< Caller-owned payload (may be NULL) */
    size_t payload_len;    /**< Payload byte length */
    uint64_t timestamp_us; /**< Creation wall-clock µs */
} eb_event_t;

/**
 * eb_event_init — initialise an event descriptor
 *
 * @param e            Event to initialise
 * @param type_id      Event type
 * @param payload      Caller-owned payload (may be NULL)
 * @param payload_len  Payload byte length
 * @param timestamp_us Creation wall-clock µs
 * @return             0 on success, -1 on NULL
 */
int eb_event_init(eb_event_t *e, eb_type_t type_id, void *payload, size_t payload_len,
                  uint64_t timestamp_us);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_EB_EVENT_H */
