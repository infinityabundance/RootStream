/*
 * reorder_buffer.h — Sequence-ordered packet reorder buffer
 *
 * Accepts incoming packets (potentially out-of-order), sorts them by
 * sequence number, and delivers them in-order.  Packets that have been
 * held longer than @timeout_us are flushed regardless of gaps.
 *
 * Sequence number comparison uses serial arithmetic (RFC 1982, 16-bit)
 * so wrap-around is handled correctly.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_REORDER_BUFFER_H
#define ROOTSTREAM_REORDER_BUFFER_H

#include "reorder_slot.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REORDER_BUFFER_CAPACITY  64     /**< Max in-flight packets */
#define REORDER_DEFAULT_TIMEOUT_US  80000ULL  /**< Default hold timeout: 80 ms */

/** Delivery callback: called once per in-order packet */
typedef void (*reorder_deliver_fn)(const reorder_slot_t *slot, void *user);

/** Opaque reorder buffer */
typedef struct reorder_buffer_s reorder_buffer_t;

/**
 * reorder_buffer_create — allocate buffer
 *
 * @param timeout_us  Hold timeout in µs before forced flush
 * @param deliver     Delivery callback (may be NULL for pull-mode)
 * @param user        User pointer passed to callback
 * @return            Non-NULL handle, or NULL on OOM/error
 */
reorder_buffer_t *reorder_buffer_create(uint64_t          timeout_us,
                                          reorder_deliver_fn deliver,
                                          void              *user);

/**
 * reorder_buffer_destroy — free buffer
 *
 * @param rb  Buffer to destroy
 */
void reorder_buffer_destroy(reorder_buffer_t *rb);

/**
 * reorder_buffer_insert — insert a packet
 *
 * @param rb          Buffer
 * @param seq         Sequence number
 * @param arrival_us  Arrival timestamp in µs
 * @param payload     Packet payload
 * @param payload_len Payload length
 * @return            0 on success, -1 if buffer full or duplicate seq
 */
int reorder_buffer_insert(reorder_buffer_t *rb,
                            uint16_t          seq,
                            uint64_t          arrival_us,
                            const uint8_t    *payload,
                            uint16_t          payload_len);

/**
 * reorder_buffer_flush — deliver all packets that are in-order or timed out
 *
 * Walks held packets in sequence order; flushes consecutive in-order
 * packets and any packet whose arrival_us + timeout_us <= now_us.
 *
 * @param rb      Buffer
 * @param now_us  Current time in µs
 * @return        Number of packets delivered
 */
int reorder_buffer_flush(reorder_buffer_t *rb, uint64_t now_us);

/**
 * reorder_buffer_count — number of packets currently held
 *
 * @param rb  Buffer
 * @return    Count
 */
int reorder_buffer_count(const reorder_buffer_t *rb);

/**
 * reorder_buffer_set_timeout — update hold timeout
 *
 * @param rb          Buffer
 * @param timeout_us  New timeout in µs (> 0)
 * @return            0 on success, -1 on invalid args
 */
int reorder_buffer_set_timeout(reorder_buffer_t *rb, uint64_t timeout_us);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_REORDER_BUFFER_H */
