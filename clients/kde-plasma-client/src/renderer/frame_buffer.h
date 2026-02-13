/**
 * @file frame_buffer.h
 * @brief Thread-safe frame buffer management for video rendering
 * 
 * Provides a lock-free ring buffer for queuing decoded video frames.
 * Supports double-buffering with frame drop detection.
 */

#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include "renderer.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Frame buffer configuration
 */
#define FRAME_BUFFER_SIZE 4  /**< Ring buffer size (double-buffer + 2 spare) */

/**
 * Frame buffer structure
 */
typedef struct {
    frame_t *frames[FRAME_BUFFER_SIZE];  /**< Frame ring buffer */
    int write_index;                      /**< Current write position */
    int read_index;                       /**< Current read position */
    uint32_t dropped_count;               /**< Number of dropped frames */
    pthread_mutex_t lock;                 /**< Thread synchronization */
} frame_buffer_t;

/**
 * Initialize frame buffer
 * 
 * @param buffer Frame buffer to initialize
 * @return 0 on success, -1 on failure
 */
int frame_buffer_init(frame_buffer_t *buffer);

/**
 * Enqueue a frame for rendering
 * 
 * If the buffer is full, the oldest frame will be dropped.
 * 
 * @param buffer Frame buffer
 * @param frame Frame to enqueue (will be copied)
 * @return 0 on success, -1 on failure
 */
int frame_buffer_enqueue(frame_buffer_t *buffer, const frame_t *frame);

/**
 * Dequeue a frame for rendering
 * 
 * @param buffer Frame buffer
 * @return Frame pointer, or NULL if buffer is empty
 */
frame_t* frame_buffer_dequeue(frame_buffer_t *buffer);

/**
 * Get number of frames currently queued
 * 
 * @param buffer Frame buffer
 * @return Number of queued frames
 */
int frame_buffer_count(frame_buffer_t *buffer);

/**
 * Clean up frame buffer
 * 
 * @param buffer Frame buffer to cleanup
 */
void frame_buffer_cleanup(frame_buffer_t *buffer);

#ifdef __cplusplus
}
#endif

#endif /* FRAME_BUFFER_H */
