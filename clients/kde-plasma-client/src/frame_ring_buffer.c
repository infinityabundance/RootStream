/**
 * @file frame_ring_buffer.c
 * @brief Lock-free ring buffer implementation for video frames
 *
 * Uses C11 _Atomic uint32_t head/tail counters with sequential-consistency
 * ordering so no additional memory barriers are needed.  The buffer is
 * single-producer / single-consumer (SPSC).
 */

#include "frame_ring_buffer.h"

#include <string.h>
#include <stdatomic.h>

void frame_ring_buffer_init(frame_ring_buffer_t *rb)
{
    if (!rb) return;
    atomic_store(&rb->head, 0);
    atomic_store(&rb->tail, 0);
}

int frame_ring_buffer_push(frame_ring_buffer_t *rb,
                            const uint8_t *y_data,
                            const uint8_t *uv_data,
                            uint32_t width,
                            uint32_t height,
                            uint64_t timestamp)
{
    if (!rb || !y_data || !uv_data) return -1;

    uint32_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    uint32_t head = atomic_load_explicit(&rb->head, memory_order_acquire);

    /* Full when distance == capacity */
    if ((tail - head) >= FRAME_RING_BUFFER_CAPACITY) return -1;

    uint32_t slot_idx = tail % FRAME_RING_BUFFER_CAPACITY;
    frame_ring_slot_t *slot = &rb->slots[slot_idx];

    /* Copy luma then interleaved chroma into the inline data buffer */
    size_t y_size  = (size_t)width * height;
    size_t uv_size = (size_t)width * (height / 2);
    size_t total   = y_size + uv_size;

    if (total > FRAME_RING_BUFFER_MAX_FRAME_SIZE) return -1;

    memcpy(slot->data,          y_data,  y_size);
    memcpy(slot->data + y_size, uv_data, uv_size);

    slot->frame.data         = slot->data;
    slot->frame.size         = (uint32_t)total;
    slot->frame.width        = width;
    slot->frame.height       = height;
    slot->frame.format       = FRAME_FORMAT_NV12;
    slot->frame.timestamp_us = timestamp;
    slot->frame.is_keyframe  = false;

    /* Publish the new tail so the consumer can see this slot */
    atomic_store_explicit(&rb->tail, tail + 1, memory_order_release);
    return 0;
}

int frame_ring_buffer_pop(frame_ring_buffer_t *rb, frame_t *out_frame)
{
    if (!rb || !out_frame) return -1;

    uint32_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    uint32_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);

    if (head == tail) return -1; /* empty */

    uint32_t slot_idx = head % FRAME_RING_BUFFER_CAPACITY;
    *out_frame = rb->slots[slot_idx].frame;

    atomic_store_explicit(&rb->head, head + 1, memory_order_release);
    return 0;
}

uint32_t frame_ring_buffer_available(const frame_ring_buffer_t *rb)
{
    if (!rb) return 0;
    uint32_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    uint32_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    return tail - head;
}

void frame_ring_buffer_cleanup(frame_ring_buffer_t *rb)
{
    /* All storage is inline – nothing to free */
    (void)rb;
}
