/**
 * @file frame_buffer.c
 * @brief Thread-safe frame buffer implementation
 */

#include "frame_buffer.h"
#include <stdlib.h>
#include <string.h>

int frame_buffer_init(frame_buffer_t *buffer) {
    if (!buffer) {
        return -1;
    }
    
    memset(buffer, 0, sizeof(frame_buffer_t));
    
    // Initialize mutex
    if (pthread_mutex_init(&buffer->lock, NULL) != 0) {
        return -1;
    }
    
    return 0;
}

int frame_buffer_enqueue(frame_buffer_t *buffer, const frame_t *frame) {
    if (!buffer || !frame) {
        return -1;
    }
    
    pthread_mutex_lock(&buffer->lock);
    
    // Calculate next write position
    int next_write = (buffer->write_index + 1) % FRAME_BUFFER_SIZE;
    
    // Check if buffer is full
    if (next_write == buffer->read_index) {
        // Buffer full, drop oldest frame
        buffer->dropped_count++;
        buffer->read_index = (buffer->read_index + 1) % FRAME_BUFFER_SIZE;
        
        // Free the dropped frame
        if (buffer->frames[buffer->read_index]) {
            free(buffer->frames[buffer->read_index]->data);
            free(buffer->frames[buffer->read_index]);
            buffer->frames[buffer->read_index] = NULL;
        }
    }
    
    // Allocate new frame
    frame_t *new_frame = (frame_t*)malloc(sizeof(frame_t));
    if (!new_frame) {
        pthread_mutex_unlock(&buffer->lock);
        return -1;
    }
    
    // Copy frame metadata
    memcpy(new_frame, frame, sizeof(frame_t));
    
    // Allocate and copy frame data
    new_frame->data = (uint8_t*)malloc(frame->size);
    if (!new_frame->data) {
        free(new_frame);
        pthread_mutex_unlock(&buffer->lock);
        return -1;
    }
    memcpy(new_frame->data, frame->data, frame->size);
    
    // Store frame in buffer
    buffer->frames[buffer->write_index] = new_frame;
    buffer->write_index = next_write;
    
    pthread_mutex_unlock(&buffer->lock);
    return 0;
}

frame_t* frame_buffer_dequeue(frame_buffer_t *buffer) {
    if (!buffer) {
        return NULL;
    }
    
    pthread_mutex_lock(&buffer->lock);
    
    // Check if buffer is empty
    if (buffer->read_index == buffer->write_index) {
        pthread_mutex_unlock(&buffer->lock);
        return NULL;
    }
    
    // Get frame from read position
    frame_t *frame = buffer->frames[buffer->read_index];
    buffer->frames[buffer->read_index] = NULL;
    
    // Advance read position
    buffer->read_index = (buffer->read_index + 1) % FRAME_BUFFER_SIZE;
    
    pthread_mutex_unlock(&buffer->lock);
    return frame;
}

int frame_buffer_count(frame_buffer_t *buffer) {
    if (!buffer) {
        return 0;
    }
    
    pthread_mutex_lock(&buffer->lock);
    
    int count;
    if (buffer->write_index >= buffer->read_index) {
        count = buffer->write_index - buffer->read_index;
    } else {
        count = FRAME_BUFFER_SIZE - buffer->read_index + buffer->write_index;
    }
    
    pthread_mutex_unlock(&buffer->lock);
    return count;
}

void frame_buffer_cleanup(frame_buffer_t *buffer) {
    if (!buffer) {
        return;
    }
    
    pthread_mutex_lock(&buffer->lock);
    
    // Free all queued frames
    for (int i = 0; i < FRAME_BUFFER_SIZE; i++) {
        if (buffer->frames[i]) {
            free(buffer->frames[i]->data);
            free(buffer->frames[i]);
            buffer->frames[i] = NULL;
        }
    }
    
    pthread_mutex_unlock(&buffer->lock);
    pthread_mutex_destroy(&buffer->lock);
}
