/*
 * bandwidth_estimator.c - Bandwidth estimation implementation
 */

#include "bandwidth_estimator.h"
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define AIMD_INCREASE_MBPS 1    /* Additive increase */
#define AIMD_DECREASE_FACTOR 0.5f  /* Multiplicative decrease */
#define SLOW_START_THRESHOLD_MBPS 10
#define MAX_BANDWIDTH_MBPS 1000

struct bandwidth_estimator {
    uint32_t bandwidth_mbps;
    uint64_t last_update_us;
    uint32_t rtt_ms;
    float packet_loss_percent;
    aimd_state_t state;
    uint32_t cwnd;  /* Congestion window */
    uint64_t total_bytes_delivered;
    pthread_mutex_t lock;
};

static uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

bandwidth_estimator_t* bandwidth_estimator_create(void) {
    bandwidth_estimator_t *estimator = calloc(1, sizeof(bandwidth_estimator_t));
    if (!estimator) {
        return NULL;
    }
    
    pthread_mutex_init(&estimator->lock, NULL);
    estimator->bandwidth_mbps = 10;  /* Start conservatively */
    estimator->state = AIMD_SLOW_START;
    estimator->cwnd = 10;
    estimator->last_update_us = get_time_us();
    
    return estimator;
}

void bandwidth_estimator_destroy(bandwidth_estimator_t *estimator) {
    if (!estimator) {
        return;
    }
    
    pthread_mutex_destroy(&estimator->lock);
    free(estimator);
}

int bandwidth_estimator_update_delivery_rate(bandwidth_estimator_t *estimator,
                                             uint64_t delivered_bytes,
                                             uint64_t delivery_time_us) {
    if (!estimator || delivery_time_us == 0) {
        return -1;
    }
    
    pthread_mutex_lock(&estimator->lock);
    
    /* Calculate instantaneous bandwidth */
    uint64_t bytes_per_sec = delivered_bytes * 1000000ULL / delivery_time_us;
    uint32_t mbps = (uint32_t)(bytes_per_sec * 8 / 1000000);
    
    /* Update estimate with EWMA */
    estimator->bandwidth_mbps = (uint32_t)(
        0.8f * estimator->bandwidth_mbps + 0.2f * mbps
    );
    
    estimator->total_bytes_delivered += delivered_bytes;
    estimator->last_update_us = get_time_us();
    
    pthread_mutex_unlock(&estimator->lock);
    return 0;
}

bool bandwidth_estimator_detect_congestion(bandwidth_estimator_t *estimator,
                                          uint32_t rtt_ms, 
                                          float packet_loss_percent) {
    if (!estimator) {
        return false;
    }
    
    pthread_mutex_lock(&estimator->lock);
    
    estimator->rtt_ms = rtt_ms;
    estimator->packet_loss_percent = packet_loss_percent;
    
    /* Detect congestion based on packet loss or high RTT */
    bool congested = (packet_loss_percent > 1.0f) || (rtt_ms > 100);
    
    pthread_mutex_unlock(&estimator->lock);
    return congested;
}

int bandwidth_estimator_aimd_increase(bandwidth_estimator_t *estimator) {
    if (!estimator) {
        return -1;
    }
    
    pthread_mutex_lock(&estimator->lock);
    
    if (estimator->state == AIMD_SLOW_START) {
        /* Exponential increase in slow start */
        estimator->bandwidth_mbps *= 2;
        estimator->cwnd *= 2;
        
        /* Transition to congestion avoidance */
        if (estimator->bandwidth_mbps >= SLOW_START_THRESHOLD_MBPS) {
            estimator->state = AIMD_CONGESTION_AVOIDANCE;
        }
    } else {
        /* Additive increase in congestion avoidance */
        estimator->bandwidth_mbps += AIMD_INCREASE_MBPS;
        estimator->cwnd += 1;
    }
    
    /* Cap at maximum */
    if (estimator->bandwidth_mbps > MAX_BANDWIDTH_MBPS) {
        estimator->bandwidth_mbps = MAX_BANDWIDTH_MBPS;
    }
    
    pthread_mutex_unlock(&estimator->lock);
    return 0;
}

int bandwidth_estimator_aimd_decrease(bandwidth_estimator_t *estimator) {
    if (!estimator) {
        return -1;
    }
    
    pthread_mutex_lock(&estimator->lock);
    
    /* Multiplicative decrease */
    estimator->bandwidth_mbps = (uint32_t)(estimator->bandwidth_mbps * AIMD_DECREASE_FACTOR);
    estimator->cwnd = (uint32_t)(estimator->cwnd * AIMD_DECREASE_FACTOR);
    
    /* Minimum bandwidth */
    if (estimator->bandwidth_mbps < 1) {
        estimator->bandwidth_mbps = 1;
    }
    if (estimator->cwnd < 1) {
        estimator->cwnd = 1;
    }
    
    /* Transition to fast recovery */
    estimator->state = AIMD_FAST_RECOVERY;
    
    pthread_mutex_unlock(&estimator->lock);
    return 0;
}

uint32_t bandwidth_estimator_get_estimated_bandwidth_mbps(bandwidth_estimator_t *estimator) {
    if (!estimator) {
        return 0;
    }
    
    pthread_mutex_lock(&estimator->lock);
    uint32_t bw = estimator->bandwidth_mbps;
    pthread_mutex_unlock(&estimator->lock);
    
    return bw;
}

bool bandwidth_estimator_is_in_slow_start(bandwidth_estimator_t *estimator) {
    if (!estimator) {
        return false;
    }
    
    pthread_mutex_lock(&estimator->lock);
    bool slow_start = (estimator->state == AIMD_SLOW_START);
    pthread_mutex_unlock(&estimator->lock);
    
    return slow_start;
}
