/*
 * dq_entry.h — Drain Queue: entry descriptor
 *
 * A drain-queue entry carries a monotonically increasing sequence
 * number, an opaque data pointer (caller-owned), the payload byte
 * length, and a flags bitmask for priority/framing hints.
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_DQ_ENTRY_H
#define ROOTSTREAM_DQ_ENTRY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Entry flags */
#define DQ_FLAG_HIGH_PRIORITY  0x01u  /**< Drain before normal-priority entries */
#define DQ_FLAG_FLUSH          0x02u  /**< Flush marker — drain all pending first */

/** Drain queue entry */
typedef struct {
    uint64_t    seq;        /**< Monotonically increasing sequence number */
    void       *data;       /**< Caller-owned payload pointer (may be NULL) */
    size_t      data_len;   /**< Payload byte length */
    uint8_t     flags;      /**< DQ_FLAG_* bitmask */
} dq_entry_t;

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_DQ_ENTRY_H */
