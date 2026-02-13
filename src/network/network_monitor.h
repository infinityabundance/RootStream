/*
 * network_monitor.h - Network condition monitoring
 * 
 * Real-time monitoring of:
 * - Round-trip time (RTT)
 * - Packet loss percentage
 * - Jitter (RTT variance)
 * - Bandwidth estimation
 * - Congestion level detection
 */

#ifndef NETWORK_MONITOR_H
#define NETWORK_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Network congestion levels */
typedef enum {
    CONGESTION_EXCELLENT = 0,   /* RTT <20ms, loss <0.1% */
    CONGESTION_GOOD = 1,        /* RTT <50ms, loss <1% */
    CONGESTION_FAIR = 2,        /* RTT <100ms, loss <2% */
    CONGESTION_POOR = 3,        /* RTT <200ms, loss <5% */
    CONGESTION_CRITICAL = 4,    /* RTT >200ms, loss >5% */
} congestion_level_t;

/* Network conditions structure */
typedef struct {
    uint32_t rtt_ms;                /* Round-trip time */
    uint32_t rtt_variance_ms;       /* Jitter */
    float packet_loss_percent;      /* Lost packets (%) */
    uint32_t bandwidth_mbps;        /* Estimated available bandwidth */
    uint64_t last_update_us;        /* Last update timestamp */
    congestion_level_t congestion_level;
} network_conditions_t;

/* Pending packet for RTT measurement */
typedef struct {
    uint32_t sequence;
    uint64_t send_time_us;
} pending_packet_t;

/* Network monitor handle */
typedef struct network_monitor network_monitor_t;

/* Initialize network monitor */
network_monitor_t* network_monitor_create(void);

/* Cleanup network monitor */
void network_monitor_destroy(network_monitor_t *monitor);

/* Record network events */
int network_monitor_record_packet_sent(network_monitor_t *monitor, 
                                       uint32_t sequence, 
                                       uint64_t timestamp_us);

int network_monitor_record_packet_ack(network_monitor_t *monitor, 
                                      uint32_t sequence, 
                                      uint64_t timestamp_us);

int network_monitor_record_packet_lost(network_monitor_t *monitor, 
                                       uint32_t sequence);

/* Bandwidth estimation */
int network_monitor_update_bandwidth_estimate(network_monitor_t *monitor,
                                              uint32_t delivered_bytes, 
                                              uint64_t delivery_time_us);

int network_monitor_estimate_bandwidth_aimd(network_monitor_t *monitor,
                                            bool congestion_detected);

/* Query current conditions */
network_conditions_t network_monitor_get_conditions(network_monitor_t *monitor);

uint32_t network_monitor_get_rtt_ms(network_monitor_t *monitor);
float network_monitor_get_packet_loss(network_monitor_t *monitor);
uint32_t network_monitor_get_bandwidth_mbps(network_monitor_t *monitor);
congestion_level_t network_monitor_get_congestion_level(network_monitor_t *monitor);

bool network_monitor_is_congested(network_monitor_t *monitor);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_MONITOR_H */
