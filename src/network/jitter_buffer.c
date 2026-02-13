/*
 * jitter_buffer.c - Packet jitter buffer implementation
 */

#include "jitter_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define MAX_BUFFER_PACKETS 100
#define MIN_TARGET_DELAY_MS 20
#define MAX_TARGET_DELAY_MS 500

typedef struct buffered_packet {
    uint8_t *data;
    size_t size;
    uint64_t rtp_timestamp;
    uint32_t sequence;
    uint64_t arrival_time_us;
    bool is_keyframe;
    bool valid;
} buffered_packet_t;

struct jitter_buffer {
    buffered_packet_t packets[MAX_BUFFER_PACKETS];
    uint32_t packet_count;
    
    uint32_t target_delay_ms;
    uint32_t max_delay_ms;
    uint64_t last_extract_time_us;
    
    uint32_t packets_received;
    uint32_t packets_dropped;
    uint32_t next_expected_seq;
    
    pthread_mutex_t lock;
};

static uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

jitter_buffer_t* jitter_buffer_create(uint32_t target_delay_ms) {
    jitter_buffer_t *buffer = calloc(1, sizeof(jitter_buffer_t));
    if (!buffer) {
        return NULL;
    }
    
    pthread_mutex_init(&buffer->lock, NULL);
    
    buffer->target_delay_ms = target_delay_ms;
    buffer->max_delay_ms = target_delay_ms * 3;
    buffer->last_extract_time_us = get_time_us();
    
    return buffer;
}

void jitter_buffer_destroy(jitter_buffer_t *buffer) {
    if (!buffer) {
        return;
    }
    
    /* Free buffered packets */
    for (uint32_t i = 0; i < MAX_BUFFER_PACKETS; i++) {
        if (buffer->packets[i].valid && buffer->packets[i].data) {
            free(buffer->packets[i].data);
        }
    }
    
    pthread_mutex_destroy(&buffer->lock);
    free(buffer);
}

int jitter_buffer_insert_packet(jitter_buffer_t *buffer,
                                const uint8_t *data,
                                size_t size,
                                uint32_t sequence,
                                uint64_t rtp_timestamp,
                                bool is_keyframe) {
    if (!buffer || !data || size == 0) {
        return -1;
    }
    
    pthread_mutex_lock(&buffer->lock);
    
    /* Check for duplicate */
    for (uint32_t i = 0; i < MAX_BUFFER_PACKETS; i++) {
        if (buffer->packets[i].valid && buffer->packets[i].sequence == sequence) {
            pthread_mutex_unlock(&buffer->lock);
            return 0;  /* Already have this packet */
        }
    }
    
    /* Find empty slot or oldest packet to replace */
    int insert_idx = -1;
    uint64_t oldest_time = UINT64_MAX;
    
    for (uint32_t i = 0; i < MAX_BUFFER_PACKETS; i++) {
        if (!buffer->packets[i].valid) {
            insert_idx = i;
            break;
        }
        if (buffer->packets[i].arrival_time_us < oldest_time) {
            oldest_time = buffer->packets[i].arrival_time_us;
            insert_idx = i;
        }
    }
    
    if (insert_idx < 0) {
        pthread_mutex_unlock(&buffer->lock);
        return -1;
    }
    
    /* Free old data if replacing */
    if (buffer->packets[insert_idx].valid && buffer->packets[insert_idx].data) {
        free(buffer->packets[insert_idx].data);
        buffer->packets_dropped++;
    }
    
    /* Allocate and copy packet data */
    uint8_t *packet_data = malloc(size);
    if (!packet_data) {
        pthread_mutex_unlock(&buffer->lock);
        return -1;
    }
    
    memcpy(packet_data, data, size);
    
    /* Store packet */
    buffer->packets[insert_idx].data = packet_data;
    buffer->packets[insert_idx].size = size;
    buffer->packets[insert_idx].sequence = sequence;
    buffer->packets[insert_idx].rtp_timestamp = rtp_timestamp;
    buffer->packets[insert_idx].is_keyframe = is_keyframe;
    buffer->packets[insert_idx].arrival_time_us = get_time_us();
    buffer->packets[insert_idx].valid = true;
    
    buffer->packet_count++;
    buffer->packets_received++;
    
    pthread_mutex_unlock(&buffer->lock);
    return 0;
}

int jitter_buffer_extract_packet(jitter_buffer_t *buffer,
                                 uint8_t **data,
                                 size_t *size,
                                 uint32_t *sequence,
                                 bool *is_keyframe) {
    if (!buffer || !data || !size || !sequence || !is_keyframe) {
        return -1;
    }
    
    pthread_mutex_lock(&buffer->lock);
    
    uint64_t now = get_time_us();
    
    /* Find oldest packet that has been buffered long enough */
    int extract_idx = -1;
    uint64_t oldest_time = UINT64_MAX;
    
    for (uint32_t i = 0; i < MAX_BUFFER_PACKETS; i++) {
        if (!buffer->packets[i].valid) {
            continue;
        }
        
        uint64_t buffered_time = now - buffer->packets[i].arrival_time_us;
        uint32_t buffered_ms = (uint32_t)(buffered_time / 1000);
        
        /* Check if packet has been buffered long enough */
        if (buffered_ms >= buffer->target_delay_ms) {
            if (buffer->packets[i].arrival_time_us < oldest_time) {
                oldest_time = buffer->packets[i].arrival_time_us;
                extract_idx = i;
            }
        }
    }
    
    if (extract_idx < 0) {
        pthread_mutex_unlock(&buffer->lock);
        return -1;  /* No packet ready */
    }
    
    /* Return packet data */
    *data = buffer->packets[extract_idx].data;
    *size = buffer->packets[extract_idx].size;
    *sequence = buffer->packets[extract_idx].sequence;
    *is_keyframe = buffer->packets[extract_idx].is_keyframe;
    
    /* Mark as extracted (caller must free) */
    buffer->packets[extract_idx].valid = false;
    buffer->packets[extract_idx].data = NULL;
    buffer->packet_count--;
    buffer->last_extract_time_us = now;
    
    pthread_mutex_unlock(&buffer->lock);
    return 0;
}

int jitter_buffer_update_target_delay(jitter_buffer_t *buffer,
                                      uint32_t rtt_ms,
                                      uint32_t jitter_ms) {
    if (!buffer) {
        return -1;
    }
    
    pthread_mutex_lock(&buffer->lock);
    
    /* Adapt target delay based on RTT and jitter */
    uint32_t new_target = rtt_ms + jitter_ms * 2;
    
    /* Clamp to reasonable bounds */
    if (new_target < MIN_TARGET_DELAY_MS) {
        new_target = MIN_TARGET_DELAY_MS;
    }
    if (new_target > MAX_TARGET_DELAY_MS) {
        new_target = MAX_TARGET_DELAY_MS;
    }
    
    /* Smooth transition */
    buffer->target_delay_ms = (buffer->target_delay_ms + new_target) / 2;
    buffer->max_delay_ms = buffer->target_delay_ms * 3;
    
    pthread_mutex_unlock(&buffer->lock);
    return 0;
}

uint32_t jitter_buffer_get_delay_ms(jitter_buffer_t *buffer) {
    if (!buffer) {
        return 0;
    }
    
    pthread_mutex_lock(&buffer->lock);
    uint32_t delay = buffer->target_delay_ms;
    pthread_mutex_unlock(&buffer->lock);
    
    return delay;
}

uint32_t jitter_buffer_get_packet_count(jitter_buffer_t *buffer) {
    if (!buffer) {
        return 0;
    }
    
    pthread_mutex_lock(&buffer->lock);
    uint32_t count = buffer->packet_count;
    pthread_mutex_unlock(&buffer->lock);
    
    return count;
}

float jitter_buffer_get_loss_rate(jitter_buffer_t *buffer) {
    if (!buffer) {
        return 0.0f;
    }
    
    pthread_mutex_lock(&buffer->lock);
    
    float loss_rate = 0.0f;
    if (buffer->packets_received > 0) {
        loss_rate = (float)buffer->packets_dropped / (float)buffer->packets_received * 100.0f;
    }
    
    pthread_mutex_unlock(&buffer->lock);
    return loss_rate;
}
