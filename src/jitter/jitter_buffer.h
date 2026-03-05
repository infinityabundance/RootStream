/*
 * jitter_buffer.h — Sorted reorder buffer with playout delay
 *
 * The jitter buffer holds incoming packets in sorted sequence-number
 * order and releases them for playout once they are at or past the
 * playout deadline (current_time >= capture_us + playout_delay_us).
 *
 * A packet that arrives after its playout deadline is counted as "late"
 * and is still enqueued if possible (late delivery is still useful for
 * re-transmit statistics), but get() will return it with the LATE flag
 * set.
 *
 * Capacity: JITTER_BUF_CAPACITY slots.  If full, the oldest packet is
 * dropped to make room (tail-drop policy).
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_JITTER_BUFFER_H
#define ROOTSTREAM_JITTER_BUFFER_H

#include "jitter_packet.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JITTER_BUF_CAPACITY  256

/** Jitter buffer playout flag: packet was delivered late */
#define JITTER_FLAG_LATE  0x01

/** Opaque jitter buffer */
typedef struct jitter_buffer_s jitter_buffer_t;

/**
 * jitter_buffer_create — allocate jitter buffer
 *
 * @param playout_delay_us  Target playout delay in µs (e.g. 50000 = 50ms)
 * @return                  Non-NULL handle, or NULL on OOM
 */
jitter_buffer_t *jitter_buffer_create(uint64_t playout_delay_us);

/**
 * jitter_buffer_destroy — free buffer
 *
 * @param buf  Buffer to destroy
 */
void jitter_buffer_destroy(jitter_buffer_t *buf);

/**
 * jitter_buffer_push — enqueue a packet (sorted by seq_num)
 *
 * @param buf  Buffer
 * @param pkt  Packet to enqueue
 * @return     0 on success, -1 on full or NULL args
 */
int jitter_buffer_push(jitter_buffer_t       *buf,
                         const jitter_packet_t *pkt);

/**
 * jitter_buffer_pop — dequeue the next packet due for playout
 *
 * A packet is due when now_us >= capture_us + playout_delay_us.
 * Returns -1 if no packet is due yet.
 *
 * @param buf     Buffer
 * @param now_us  Current wall-clock time in µs
 * @param out     Output packet
 * @return        0 if a packet was returned, -1 otherwise
 */
int jitter_buffer_pop(jitter_buffer_t *buf,
                        uint64_t         now_us,
                        jitter_packet_t *out);

/**
 * jitter_buffer_peek — examine the next packet without dequeueing it
 *
 * @param buf  Buffer
 * @param out  Output packet
 * @return     0 on success, -1 if empty
 */
int jitter_buffer_peek(const jitter_buffer_t *buf, jitter_packet_t *out);

/**
 * jitter_buffer_count — number of packets in buffer
 *
 * @param buf  Buffer
 * @return     Packet count
 */
size_t jitter_buffer_count(const jitter_buffer_t *buf);

/**
 * jitter_buffer_is_empty — return true if buffer has no packets
 *
 * @param buf  Buffer
 * @return     true if empty
 */
bool jitter_buffer_is_empty(const jitter_buffer_t *buf);

/**
 * jitter_buffer_flush — discard all packets
 *
 * @param buf  Buffer
 */
void jitter_buffer_flush(jitter_buffer_t *buf);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_JITTER_BUFFER_H */
