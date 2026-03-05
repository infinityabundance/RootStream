/*
 * pq_heap.h — Priority Queue: 64-slot binary min-heap
 *
 * A standard array-backed binary min-heap.  Entries with smaller keys
 * are dequeued first.  The heap is statically bounded to PQ_MAX_SIZE
 * entries; overflow is tracked in the stats module.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_PQ_HEAP_H
#define ROOTSTREAM_PQ_HEAP_H

#include "pq_entry.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PQ_MAX_SIZE  64   /**< Maximum entries in the heap */

/** Opaque priority queue */
typedef struct pq_heap_s pq_heap_t;

/**
 * pq_heap_create — allocate heap
 *
 * @return Non-NULL handle, or NULL on OOM
 */
pq_heap_t *pq_heap_create(void);

/**
 * pq_heap_destroy — free heap
 */
void pq_heap_destroy(pq_heap_t *h);

/**
 * pq_heap_push — insert an entry
 *
 * @param h  Heap
 * @param e  Entry to copy into the heap
 * @return   0 on success, -1 if heap full or NULL
 */
int pq_heap_push(pq_heap_t *h, const pq_entry_t *e);

/**
 * pq_heap_pop — remove and return the minimum-key entry
 *
 * @param h    Heap
 * @param out  Receives the popped entry
 * @return     0 on success, -1 if empty or NULL
 */
int pq_heap_pop(pq_heap_t *h, pq_entry_t *out);

/**
 * pq_heap_peek — inspect the minimum-key entry without removing it
 *
 * @param h    Heap
 * @param out  Receives a copy of the minimum entry
 * @return     0 on success, -1 if empty or NULL
 */
int pq_heap_peek(const pq_heap_t *h, pq_entry_t *out);

/**
 * pq_heap_count — number of entries currently in the heap
 */
int pq_heap_count(const pq_heap_t *h);

/**
 * pq_heap_clear — remove all entries
 */
void pq_heap_clear(pq_heap_t *h);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PQ_HEAP_H */
