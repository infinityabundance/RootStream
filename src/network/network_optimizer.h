/*
 * network_optimizer.h - Main network optimization coordinator
 * 
 * Integrates all network optimization components:
 * - Network monitoring
 * - Adaptive bitrate control
 * - QoS management
 * - Bandwidth estimation
 * - Socket tuning
 */

#ifndef NETWORK_OPTIMIZER_H
#define NETWORK_OPTIMIZER_H

#include "network_monitor.h"
#include "adaptive_bitrate.h"
#include "qos_manager.h"
#include "bandwidth_estimator.h"
#include "socket_tuning.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Network optimizer handle */
typedef struct network_optimizer network_optimizer_t;

/* Network optimizer callbacks */
typedef struct {
    void (*on_bitrate_changed)(void *user_data, uint32_t new_bitrate_kbps);
    void (*on_congestion_detected)(void *user_data);
    void (*on_network_degraded)(void *user_data);
    void (*on_network_recovered)(void *user_data);
    void *user_data;
} network_optimizer_callbacks_t;

/* Create network optimizer */
network_optimizer_t* network_optimizer_create(void);

/* Destroy network optimizer */
void network_optimizer_destroy(network_optimizer_t *optimizer);

/* Initialize optimizer with callbacks */
int network_optimizer_init(network_optimizer_t *optimizer, 
                          const network_optimizer_callbacks_t *callbacks);

/* Set up default bitrate profiles */
int network_optimizer_setup_default_profiles(network_optimizer_t *optimizer);

/* Add custom bitrate profile */
int network_optimizer_add_profile(network_optimizer_t *optimizer,
                                 uint32_t bitrate_kbps,
                                 uint32_t width,
                                 uint32_t height,
                                 uint32_t fps,
                                 const char *codec,
                                 const char *preset);

/* Optimize network settings based on current conditions */
int network_optimizer_optimize(network_optimizer_t *optimizer);

/* Get current network conditions */
network_conditions_t network_optimizer_get_conditions(network_optimizer_t *optimizer);

/* Get recommended bitrate */
uint32_t network_optimizer_get_recommended_bitrate(network_optimizer_t *optimizer);

/* Record network events (for monitoring) */
int network_optimizer_record_packet_sent(network_optimizer_t *optimizer, 
                                         uint32_t sequence, 
                                         uint64_t timestamp_us);

int network_optimizer_record_packet_ack(network_optimizer_t *optimizer, 
                                        uint32_t sequence, 
                                        uint64_t timestamp_us);

int network_optimizer_record_packet_lost(network_optimizer_t *optimizer, 
                                         uint32_t sequence);

/* Tune socket for optimal performance */
int network_optimizer_tune_socket(network_optimizer_t *optimizer, 
                                  int socket, 
                                  bool low_latency);

/* Get diagnostics report as JSON string (caller must free) */
char* network_optimizer_get_diagnostics_json(network_optimizer_t *optimizer);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_OPTIMIZER_H */
