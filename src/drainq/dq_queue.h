/*
 * dq_queue.h — Drain Queue: 128-slot FIFO
 *
 * A bounded FIFO queue designed for draining encoded/processed data
 * toward a consumer (e.g., network sender).  Entries are enqueued by
 * producers and dequeued individually or bulk-drained via a callback.
 *
 * The queue assigns a monotonically increasing sequence number to each
 * enqueued entry.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_DQ_QUEUE_H
#define ROOTSTREAM_DQ_QUEUE_H

#include <stddef.h>

#include "dq_entry.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DQ_MAX_ENTRIES 128 /**< Maximum queued entries */

/** Drain callback used by dq_queue_drain_all() */
typedef void (*dq_drain_fn)(const dq_entry_t *e, void *user);

/** Opaque drain queue */
typedef struct dq_queue_s dq_queue_t;

/**
 * dq_queue_create — allocate queue
 *
 * @return Non-NULL, or NULL on OOM
 */
dq_queue_t *dq_queue_create(void);

/**
 * dq_queue_destroy — free queue (does NOT free entry payloads)
 */
void dq_queue_destroy(dq_queue_t *q);

/**
 * dq_queue_enqueue — add entry to the tail of the queue
 *
 * Assigns the next sequence number to *e before storing.
 *
 * @param q  Queue
 * @param e  Entry to copy (caller-owned payload pointer stored as-is)
 * @return   0 on success, -1 if queue full or NULL
 */
int dq_queue_enqueue(dq_queue_t *q, const dq_entry_t *e);

/**
 * dq_queue_dequeue — remove and return the head entry
 *
 * @param q    Queue
 * @param out  Receives the dequeued entry
 * @return     0 on success, -1 if empty or NULL
 */
int dq_queue_dequeue(dq_queue_t *q, dq_entry_t *out);

/**
 * dq_queue_drain_all — dequeue all entries invoking cb for each
 *
 * @param q    Queue
 * @param cb   Called for each entry in FIFO order (may be NULL)
 * @param user Passed through to cb
 * @return     Number of entries drained
 */
int dq_queue_drain_all(dq_queue_t *q, dq_drain_fn cb, void *user);

/**
 * dq_queue_count — current number of queued entries
 */
int dq_queue_count(const dq_queue_t *q);

/**
 * dq_queue_clear — discard all entries without invoking any callback
 */
void dq_queue_clear(dq_queue_t *q);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_DQ_QUEUE_H */
