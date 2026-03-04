/**
 * @file frame_ring_buffer.h
 * @brief Lock-free ring buffer for video frames between decode and render threads
 *
 * Provides a fixed-capacity, lock-free ring buffer using C11 atomic head/tail
 * counters. The producer (decode thread) calls frame_ring_buffer_push() and the
 * consumer (Vulkan render thread) calls frame_ring_buffer_pop(). Frames are
 * dropped (push returns -1) when the buffer is full rather than blocking.
 *
 * Capacity is fixed at FRAME_RING_BUFFER_CAPACITY (4) slots.
 */

#ifndef FRAME_RING_BUFFER_H
#define FRAME_RING_BUFFER_H

#include <stdint.h>
#include <stddef.h>

/* _Atomic is C11; in C++ we only need layout compatibility since the
 * atomic ops are implemented in the C translation unit. */
#ifdef __cplusplus
#  define FRAME_RB_ATOMIC(T) T
#else
#  include <stdatomic.h>
#  define FRAME_RB_ATOMIC(T) _Atomic T
#endif

#include "renderer/renderer.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Number of frame slots in the ring buffer. */
#define FRAME_RING_BUFFER_CAPACITY 4

/** Maximum bytes allocated per frame data buffer (4K NV12 = ~12 MB). */
#define FRAME_RING_BUFFER_MAX_FRAME_SIZE (3840 * 2160 * 3 / 2)

/**
 * Per-slot storage: a frame_t descriptor plus its backing data buffer.
 */
typedef struct {
    frame_t   frame;                              /**< Frame metadata and data pointer */
    uint8_t   data[FRAME_RING_BUFFER_MAX_FRAME_SIZE]; /**< Backing pixel data */
} frame_ring_slot_t;

/**
 * Lock-free ring buffer for video frames.
 *
 * head  – next slot to read  (consumer advances)
 * tail  – next slot to write (producer advances)
 *
 * The buffer is full when (tail - head) == FRAME_RING_BUFFER_CAPACITY.
 * The buffer is empty when head == tail.
 */
typedef struct {
    frame_ring_slot_t   slots[FRAME_RING_BUFFER_CAPACITY]; /**< Frame storage slots */
    FRAME_RB_ATOMIC(uint32_t)   head;                               /**< Consumer read index */
    FRAME_RB_ATOMIC(uint32_t)   tail;                               /**< Producer write index */
} frame_ring_buffer_t;

/**
 * Initialize a ring buffer instance.
 *
 * Zeroes the atomic counters; the data arrays need not be zeroed explicitly.
 *
 * @param rb  Ring buffer to initialise (must not be NULL)
 */
void frame_ring_buffer_init(frame_ring_buffer_t *rb);

/**
 * Push a frame into the ring buffer (non-blocking, called from decode thread).
 *
 * Copies \p width * \p height bytes of luma (\p y_data) followed by
 * \p width * (\p height / 2) bytes of interleaved chroma (\p uv_data) into
 * the next available slot and advances the tail counter.
 *
 * @param rb         Ring buffer
 * @param y_data     Luma plane pointer
 * @param uv_data    Interleaved chroma plane pointer
 * @param width      Frame width in pixels
 * @param height     Frame height in pixels
 * @param timestamp  Presentation timestamp in microseconds
 * @return 0 on success, -1 if the buffer is full (frame dropped)
 */
int frame_ring_buffer_push(frame_ring_buffer_t *rb,
                            const uint8_t *y_data,
                            const uint8_t *uv_data,
                            uint32_t width,
                            uint32_t height,
                            uint64_t timestamp);

/**
 * Pop the oldest frame from the ring buffer (non-blocking, called from render thread).
 *
 * Copies the frame descriptor into \p out_frame.  The data pointer in
 * \p out_frame->data points into the ring buffer slot and remains valid until
 * the next call to frame_ring_buffer_pop() that wraps around to the same slot
 * (i.e. after FRAME_RING_BUFFER_CAPACITY pops).
 *
 * @param rb         Ring buffer
 * @param out_frame  Destination frame descriptor
 * @return 0 on success, -1 if the buffer is empty
 */
int frame_ring_buffer_pop(frame_ring_buffer_t *rb, frame_t *out_frame);

/**
 * Return the number of frames currently available for popping.
 *
 * @param rb  Ring buffer
 * @return Number of frames ready to consume (0 to FRAME_RING_BUFFER_CAPACITY)
 */
uint32_t frame_ring_buffer_available(const frame_ring_buffer_t *rb);

/**
 * Release any resources associated with the ring buffer.
 *
 * Currently a no-op because all storage is inline; provided for symmetry with
 * frame_ring_buffer_init().
 *
 * @param rb  Ring buffer
 */
void frame_ring_buffer_cleanup(frame_ring_buffer_t *rb);

#ifdef __cplusplus
}
#endif

#endif /* FRAME_RING_BUFFER_H */
