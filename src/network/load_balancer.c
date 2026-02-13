/*
 * load_balancer.c - Multi-stream load balancing implementation
 */

#include "load_balancer.h"
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#define MAX_STREAMS 16

typedef struct {
    uint32_t stream_id;
    uint32_t bitrate_kbps;
    uint32_t packets_in_flight;
    uint64_t bytes_sent;
    float loss_rate;
    uint32_t rtt_ms;
    bool active;
} stream_state_t;

struct load_balancer {
    stream_state_t streams[MAX_STREAMS];
    uint32_t stream_count;
    uint32_t total_available_bandwidth_mbps;
    pthread_mutex_t lock;
};

load_balancer_t* load_balancer_create(void) {
    load_balancer_t *balancer = calloc(1, sizeof(load_balancer_t));
    if (!balancer) {
        return NULL;
    }
    
    pthread_mutex_init(&balancer->lock, NULL);
    balancer->total_available_bandwidth_mbps = 100;  /* Default */
    
    return balancer;
}

void load_balancer_destroy(load_balancer_t *balancer) {
    if (!balancer) {
        return;
    }
    
    pthread_mutex_destroy(&balancer->lock);
    free(balancer);
}

int load_balancer_add_stream(load_balancer_t *balancer, 
                             uint32_t stream_id, 
                             uint32_t initial_bitrate_kbps) {
    if (!balancer) {
        return -1;
    }
    
    pthread_mutex_lock(&balancer->lock);
    
    /* Find free slot */
    for (uint32_t i = 0; i < MAX_STREAMS; i++) {
        if (!balancer->streams[i].active) {
            balancer->streams[i].stream_id = stream_id;
            balancer->streams[i].bitrate_kbps = initial_bitrate_kbps;
            balancer->streams[i].active = true;
            balancer->stream_count++;
            pthread_mutex_unlock(&balancer->lock);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&balancer->lock);
    return -1;  /* No free slots */
}

int load_balancer_remove_stream(load_balancer_t *balancer, uint32_t stream_id) {
    if (!balancer) {
        return -1;
    }
    
    pthread_mutex_lock(&balancer->lock);
    
    for (uint32_t i = 0; i < MAX_STREAMS; i++) {
        if (balancer->streams[i].active && 
            balancer->streams[i].stream_id == stream_id) {
            balancer->streams[i].active = false;
            balancer->stream_count--;
            pthread_mutex_unlock(&balancer->lock);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&balancer->lock);
    return -1;  /* Stream not found */
}

int load_balancer_allocate_bandwidth(load_balancer_t *balancer, 
                                     uint32_t total_bandwidth_mbps) {
    if (!balancer) {
        return -1;
    }
    
    pthread_mutex_lock(&balancer->lock);
    
    balancer->total_available_bandwidth_mbps = total_bandwidth_mbps;
    
    /* Fair share allocation */
    if (balancer->stream_count > 0) {
        uint32_t per_stream_kbps = (total_bandwidth_mbps * 1000) / balancer->stream_count;
        
        for (uint32_t i = 0; i < MAX_STREAMS; i++) {
            if (balancer->streams[i].active) {
                balancer->streams[i].bitrate_kbps = per_stream_kbps;
            }
        }
    }
    
    pthread_mutex_unlock(&balancer->lock);
    return 0;
}

uint32_t load_balancer_get_stream_bitrate(load_balancer_t *balancer, 
                                          uint32_t stream_id) {
    if (!balancer) {
        return 0;
    }
    
    pthread_mutex_lock(&balancer->lock);
    
    for (uint32_t i = 0; i < MAX_STREAMS; i++) {
        if (balancer->streams[i].active && 
            balancer->streams[i].stream_id == stream_id) {
            uint32_t bitrate = balancer->streams[i].bitrate_kbps;
            pthread_mutex_unlock(&balancer->lock);
            return bitrate;
        }
    }
    
    pthread_mutex_unlock(&balancer->lock);
    return 0;
}

int load_balancer_allocate_fair_share(load_balancer_t *balancer) {
    if (!balancer) {
        return -1;
    }
    
    return load_balancer_allocate_bandwidth(balancer, 
                                           balancer->total_available_bandwidth_mbps);
}

uint32_t load_balancer_get_stream_count(load_balancer_t *balancer) {
    if (!balancer) {
        return 0;
    }
    
    pthread_mutex_lock(&balancer->lock);
    uint32_t count = balancer->stream_count;
    pthread_mutex_unlock(&balancer->lock);
    
    return count;
}
