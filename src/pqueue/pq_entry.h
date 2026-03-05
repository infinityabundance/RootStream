/*
 * pq_entry.h — Priority Queue: entry descriptor
 *
 * Each entry in the priority queue carries a 64-bit key (deadline_us
 * or any other orderable priority), an opaque data pointer (caller-
 * owned), and a 32-bit application identifier.
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_PQ_ENTRY_H
#define ROOTSTREAM_PQ_ENTRY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Priority queue entry (lower key = higher priority) */
typedef struct {
    uint64_t key;    /**< Sort key; smallest key is dequeued first */
    void    *data;   /**< Caller-owned data pointer (may be NULL) */
    uint32_t id;     /**< Application identifier */
} pq_entry_t;

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PQ_ENTRY_H */
