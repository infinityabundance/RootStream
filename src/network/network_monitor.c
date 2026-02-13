/*
 * network_monitor.c - Network condition monitoring implementation
 */

#include "network_monitor.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#define MAX_PENDING_PACKETS 1000
#define RTT_SMOOTH_FACTOR 0.125f    /* 1/8 for EWMA */
#define PACKET_LOSS_WINDOW 100      /* Track last 100 packets */

struct network_monitor {
    network_conditions_t conditions;
    pthread_mutex_t lock;
    
    /* RTT measurement */
    pending_packet_t pending_packets[MAX_PENDING_PACKETS];
    uint32_t pending_count;
    uint32_t rtt_samples;
    float rtt_ewma;
    float rtt_var_ewma;
    
    /* Packet loss tracking */
    uint32_t packets_sent;
    uint32_t packets_acked;
    uint32_t packets_lost;
    uint32_t loss_window_sent;
    uint32_t loss_window_lost;
    
    /* Bandwidth estimation */
    uint32_t estimated_bw_mbps;
    uint64_t bw_estimate_time_us;
    uint64_t total_bytes_delivered;
};

/* Get current time in microseconds */
static uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

/* Update congestion level based on RTT and packet loss */
static void update_congestion_level(network_monitor_t *monitor) {
    uint32_t rtt = monitor->conditions.rtt_ms;
    float loss = monitor->conditions.packet_loss_percent;
    
    if (rtt < 20 && loss < 0.1f) {
        monitor->conditions.congestion_level = CONGESTION_EXCELLENT;
    } else if (rtt < 50 && loss < 1.0f) {
        monitor->conditions.congestion_level = CONGESTION_GOOD;
    } else if (rtt < 100 && loss < 2.0f) {
        monitor->conditions.congestion_level = CONGESTION_FAIR;
    } else if (rtt < 200 && loss < 5.0f) {
        monitor->conditions.congestion_level = CONGESTION_POOR;
    } else {
        monitor->conditions.congestion_level = CONGESTION_CRITICAL;
    }
}

network_monitor_t* network_monitor_create(void) {
    network_monitor_t *monitor = calloc(1, sizeof(network_monitor_t));
    if (!monitor) {
        return NULL;
    }
    
    pthread_mutex_init(&monitor->lock, NULL);
    
    /* Initialize with reasonable defaults */
    monitor->conditions.rtt_ms = 20;
    monitor->conditions.rtt_variance_ms = 5;
    monitor->conditions.packet_loss_percent = 0.0f;
    monitor->conditions.bandwidth_mbps = 100;  /* Assume 100 Mbps initially */
    monitor->conditions.congestion_level = CONGESTION_GOOD;
    monitor->conditions.last_update_us = get_time_us();
    
    monitor->rtt_ewma = 20.0f;
    monitor->rtt_var_ewma = 5.0f;
    monitor->estimated_bw_mbps = 100;
    
    return monitor;
}

void network_monitor_destroy(network_monitor_t *monitor) {
    if (!monitor) {
        return;
    }
    
    pthread_mutex_destroy(&monitor->lock);
    free(monitor);
}

int network_monitor_record_packet_sent(network_monitor_t *monitor, 
                                       uint32_t sequence, 
                                       uint64_t timestamp_us) {
    if (!monitor) {
        return -1;
    }
    
    pthread_mutex_lock(&monitor->lock);
    
    /* Add to pending packets if not full */
    if (monitor->pending_count < MAX_PENDING_PACKETS) {
        pending_packet_t *pkt = &monitor->pending_packets[monitor->pending_count++];
        pkt->sequence = sequence;
        pkt->send_time_us = timestamp_us;
    }
    
    monitor->packets_sent++;
    monitor->loss_window_sent++;
    
    /* Reset loss window periodically */
    if (monitor->loss_window_sent > PACKET_LOSS_WINDOW) {
        monitor->loss_window_sent = 0;
        monitor->loss_window_lost = 0;
    }
    
    pthread_mutex_unlock(&monitor->lock);
    return 0;
}

int network_monitor_record_packet_ack(network_monitor_t *monitor, 
                                      uint32_t sequence, 
                                      uint64_t timestamp_us) {
    if (!monitor) {
        return -1;
    }
    
    pthread_mutex_lock(&monitor->lock);
    
    /* Find matching pending packet */
    for (uint32_t i = 0; i < monitor->pending_count; i++) {
        if (monitor->pending_packets[i].sequence == sequence) {
            /* Calculate RTT */
            uint64_t rtt_us = timestamp_us - monitor->pending_packets[i].send_time_us;
            float rtt_ms = (float)rtt_us / 1000.0f;
            
            /* Update RTT using EWMA (Exponential Weighted Moving Average) */
            if (monitor->rtt_samples == 0) {
                monitor->rtt_ewma = rtt_ms;
                monitor->rtt_var_ewma = rtt_ms / 2.0f;
            } else {
                float delta = fabs(rtt_ms - monitor->rtt_ewma);
                monitor->rtt_ewma = (1.0f - RTT_SMOOTH_FACTOR) * monitor->rtt_ewma + 
                                   RTT_SMOOTH_FACTOR * rtt_ms;
                monitor->rtt_var_ewma = (1.0f - RTT_SMOOTH_FACTOR) * monitor->rtt_var_ewma + 
                                       RTT_SMOOTH_FACTOR * delta;
            }
            
            monitor->rtt_samples++;
            monitor->conditions.rtt_ms = (uint32_t)monitor->rtt_ewma;
            monitor->conditions.rtt_variance_ms = (uint32_t)monitor->rtt_var_ewma;
            
            /* Remove from pending list (shift remaining) */
            for (uint32_t j = i; j < monitor->pending_count - 1; j++) {
                monitor->pending_packets[j] = monitor->pending_packets[j + 1];
            }
            monitor->pending_count--;
            
            break;
        }
    }
    
    monitor->packets_acked++;
    
    /* Update packet loss percentage */
    if (monitor->packets_sent > 0) {
        float total_loss_rate = (float)(monitor->packets_sent - monitor->packets_acked) / 
                                (float)monitor->packets_sent * 100.0f;
        monitor->conditions.packet_loss_percent = total_loss_rate;
    }
    
    /* Update congestion level */
    update_congestion_level(monitor);
    monitor->conditions.last_update_us = timestamp_us;
    
    pthread_mutex_unlock(&monitor->lock);
    return 0;
}

int network_monitor_record_packet_lost(network_monitor_t *monitor, 
                                       uint32_t sequence) {
    if (!monitor) {
        return -1;
    }
    
    pthread_mutex_lock(&monitor->lock);
    
    /* Remove from pending list */
    for (uint32_t i = 0; i < monitor->pending_count; i++) {
        if (monitor->pending_packets[i].sequence == sequence) {
            for (uint32_t j = i; j < monitor->pending_count - 1; j++) {
                monitor->pending_packets[j] = monitor->pending_packets[j + 1];
            }
            monitor->pending_count--;
            break;
        }
    }
    
    monitor->packets_lost++;
    monitor->loss_window_lost++;
    
    /* Update packet loss percentage */
    if (monitor->loss_window_sent > 0) {
        monitor->conditions.packet_loss_percent = 
            (float)monitor->loss_window_lost / (float)monitor->loss_window_sent * 100.0f;
    }
    
    /* Update congestion level */
    update_congestion_level(monitor);
    monitor->conditions.last_update_us = get_time_us();
    
    pthread_mutex_unlock(&monitor->lock);
    return 0;
}

int network_monitor_update_bandwidth_estimate(network_monitor_t *monitor,
                                              uint32_t delivered_bytes, 
                                              uint64_t delivery_time_us) {
    if (!monitor || delivery_time_us == 0) {
        return -1;
    }
    
    pthread_mutex_lock(&monitor->lock);
    
    /* Calculate instantaneous bandwidth */
    uint64_t bytes_per_sec = (uint64_t)delivered_bytes * 1000000ULL / delivery_time_us;
    uint32_t mbps = (uint32_t)(bytes_per_sec * 8 / 1000000);
    
    /* Smooth bandwidth estimate using EWMA */
    if (monitor->bw_estimate_time_us == 0) {
        monitor->estimated_bw_mbps = mbps;
    } else {
        monitor->estimated_bw_mbps = (uint32_t)(
            0.8f * monitor->estimated_bw_mbps + 0.2f * mbps
        );
    }
    
    monitor->conditions.bandwidth_mbps = monitor->estimated_bw_mbps;
    monitor->bw_estimate_time_us = get_time_us();
    monitor->total_bytes_delivered += delivered_bytes;
    
    pthread_mutex_unlock(&monitor->lock);
    return 0;
}

int network_monitor_estimate_bandwidth_aimd(network_monitor_t *monitor,
                                            bool congestion_detected) {
    if (!monitor) {
        return -1;
    }
    
    pthread_mutex_lock(&monitor->lock);
    
    if (congestion_detected) {
        /* Multiplicative decrease: reduce by 50% */
        monitor->estimated_bw_mbps = monitor->estimated_bw_mbps / 2;
        if (monitor->estimated_bw_mbps < 1) {
            monitor->estimated_bw_mbps = 1;
        }
    } else {
        /* Additive increase: add 1 Mbps */
        monitor->estimated_bw_mbps += 1;
        /* Cap at reasonable maximum */
        if (monitor->estimated_bw_mbps > 1000) {
            monitor->estimated_bw_mbps = 1000;
        }
    }
    
    monitor->conditions.bandwidth_mbps = monitor->estimated_bw_mbps;
    
    pthread_mutex_unlock(&monitor->lock);
    return 0;
}

network_conditions_t network_monitor_get_conditions(network_monitor_t *monitor) {
    network_conditions_t conditions;
    memset(&conditions, 0, sizeof(conditions));
    
    if (!monitor) {
        return conditions;
    }
    
    pthread_mutex_lock(&monitor->lock);
    conditions = monitor->conditions;
    pthread_mutex_unlock(&monitor->lock);
    
    return conditions;
}

uint32_t network_monitor_get_rtt_ms(network_monitor_t *monitor) {
    return network_monitor_get_conditions(monitor).rtt_ms;
}

float network_monitor_get_packet_loss(network_monitor_t *monitor) {
    return network_monitor_get_conditions(monitor).packet_loss_percent;
}

uint32_t network_monitor_get_bandwidth_mbps(network_monitor_t *monitor) {
    return network_monitor_get_conditions(monitor).bandwidth_mbps;
}

congestion_level_t network_monitor_get_congestion_level(network_monitor_t *monitor) {
    return network_monitor_get_conditions(monitor).congestion_level;
}

bool network_monitor_is_congested(network_monitor_t *monitor) {
    congestion_level_t level = network_monitor_get_congestion_level(monitor);
    return level >= CONGESTION_FAIR;
}
