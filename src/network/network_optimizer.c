/*
 * network_optimizer.c - Main network optimization coordinator implementation
 */

#include "network_optimizer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

struct network_optimizer {
    network_monitor_t *monitor;
    abr_controller_t *abr;
    qos_manager_t *qos;
    bandwidth_estimator_t *bandwidth_est;
    socket_tuning_t *socket_tuning;
    
    network_optimizer_callbacks_t callbacks;
    
    congestion_level_t last_congestion_level;
    uint32_t last_bitrate_kbps;
    
    uint64_t optimization_count;
    pthread_mutex_t lock;
};

network_optimizer_t* network_optimizer_create(void) {
    network_optimizer_t *optimizer = calloc(1, sizeof(network_optimizer_t));
    if (!optimizer) {
        return NULL;
    }
    
    /* Create sub-components */
    optimizer->monitor = network_monitor_create();
    optimizer->abr = abr_controller_create(optimizer->monitor);
    optimizer->qos = qos_manager_create();
    optimizer->bandwidth_est = bandwidth_estimator_create();
    optimizer->socket_tuning = socket_tuning_create();
    
    if (!optimizer->monitor || !optimizer->abr || !optimizer->qos || 
        !optimizer->bandwidth_est || !optimizer->socket_tuning) {
        network_optimizer_destroy(optimizer);
        return NULL;
    }
    
    pthread_mutex_init(&optimizer->lock, NULL);
    
    optimizer->last_congestion_level = CONGESTION_GOOD;
    optimizer->last_bitrate_kbps = 5000;
    
    return optimizer;
}

void network_optimizer_destroy(network_optimizer_t *optimizer) {
    if (!optimizer) {
        return;
    }
    
    if (optimizer->monitor) {
        network_monitor_destroy(optimizer->monitor);
    }
    if (optimizer->abr) {
        abr_controller_destroy(optimizer->abr);
    }
    if (optimizer->qos) {
        qos_manager_destroy(optimizer->qos);
    }
    if (optimizer->bandwidth_est) {
        bandwidth_estimator_destroy(optimizer->bandwidth_est);
    }
    if (optimizer->socket_tuning) {
        socket_tuning_destroy(optimizer->socket_tuning);
    }
    
    pthread_mutex_destroy(&optimizer->lock);
    free(optimizer);
}

int network_optimizer_init(network_optimizer_t *optimizer, 
                          const network_optimizer_callbacks_t *callbacks) {
    if (!optimizer) {
        return -1;
    }
    
    if (callbacks) {
        optimizer->callbacks = *callbacks;
    }
    
    return 0;
}

int network_optimizer_setup_default_profiles(network_optimizer_t *optimizer) {
    if (!optimizer || !optimizer->abr) {
        return -1;
    }
    
    /* Add default quality profiles */
    abr_controller_add_profile(optimizer->abr, 500, 640, 480, 30, "H.264", "fast");
    abr_controller_add_profile(optimizer->abr, 1500, 1280, 720, 30, "H.264", "fast");
    abr_controller_add_profile(optimizer->abr, 3000, 1280, 720, 60, "H.264", "medium");
    abr_controller_add_profile(optimizer->abr, 5000, 1920, 1080, 30, "H.264", "medium");
    abr_controller_add_profile(optimizer->abr, 8000, 1920, 1080, 60, "H.264", "medium");
    abr_controller_add_profile(optimizer->abr, 15000, 2560, 1440, 60, "H.264", "medium");
    abr_controller_add_profile(optimizer->abr, 25000, 3840, 2160, 30, "H.264", "slow");
    
    return 0;
}

int network_optimizer_add_profile(network_optimizer_t *optimizer,
                                 uint32_t bitrate_kbps,
                                 uint32_t width,
                                 uint32_t height,
                                 uint32_t fps,
                                 const char *codec,
                                 const char *preset) {
    if (!optimizer || !optimizer->abr) {
        return -1;
    }
    
    return abr_controller_add_profile(optimizer->abr, bitrate_kbps, width, height, 
                                     fps, codec, preset);
}

int network_optimizer_optimize(network_optimizer_t *optimizer) {
    if (!optimizer) {
        return -1;
    }
    
    pthread_mutex_lock(&optimizer->lock);
    
    /* Get current network conditions */
    network_conditions_t conditions = network_monitor_get_conditions(optimizer->monitor);
    
    /* Update bandwidth estimation based on congestion */
    bool congested = bandwidth_estimator_detect_congestion(
        optimizer->bandwidth_est, 
        conditions.rtt_ms, 
        conditions.packet_loss_percent
    );
    
    if (congested) {
        bandwidth_estimator_aimd_decrease(optimizer->bandwidth_est);
    } else {
        bandwidth_estimator_aimd_increase(optimizer->bandwidth_est);
    }
    
    /* Get recommended bitrate profile */
    const bitrate_profile_t *profile = abr_controller_get_recommended_profile(optimizer->abr);
    
    /* Trigger callbacks if state changed */
    if (profile && profile->bitrate_kbps != optimizer->last_bitrate_kbps) {
        optimizer->last_bitrate_kbps = profile->bitrate_kbps;
        
        if (optimizer->callbacks.on_bitrate_changed) {
            optimizer->callbacks.on_bitrate_changed(
                optimizer->callbacks.user_data, 
                profile->bitrate_kbps
            );
        }
    }
    
    if (congested && optimizer->callbacks.on_congestion_detected) {
        optimizer->callbacks.on_congestion_detected(optimizer->callbacks.user_data);
    }
    
    if (conditions.congestion_level > optimizer->last_congestion_level) {
        if (optimizer->callbacks.on_network_degraded) {
            optimizer->callbacks.on_network_degraded(optimizer->callbacks.user_data);
        }
    } else if (conditions.congestion_level < optimizer->last_congestion_level) {
        if (optimizer->callbacks.on_network_recovered) {
            optimizer->callbacks.on_network_recovered(optimizer->callbacks.user_data);
        }
    }
    
    optimizer->last_congestion_level = conditions.congestion_level;
    optimizer->optimization_count++;
    
    pthread_mutex_unlock(&optimizer->lock);
    return 0;
}

network_conditions_t network_optimizer_get_conditions(network_optimizer_t *optimizer) {
    network_conditions_t conditions;
    memset(&conditions, 0, sizeof(conditions));
    
    if (!optimizer || !optimizer->monitor) {
        return conditions;
    }
    
    return network_monitor_get_conditions(optimizer->monitor);
}

uint32_t network_optimizer_get_recommended_bitrate(network_optimizer_t *optimizer) {
    if (!optimizer || !optimizer->abr) {
        return 0;
    }
    
    const bitrate_profile_t *profile = abr_controller_get_recommended_profile(optimizer->abr);
    return profile ? profile->bitrate_kbps : 0;
}

int network_optimizer_record_packet_sent(network_optimizer_t *optimizer, 
                                         uint32_t sequence, 
                                         uint64_t timestamp_us) {
    if (!optimizer || !optimizer->monitor) {
        return -1;
    }
    
    return network_monitor_record_packet_sent(optimizer->monitor, sequence, timestamp_us);
}

int network_optimizer_record_packet_ack(network_optimizer_t *optimizer, 
                                        uint32_t sequence, 
                                        uint64_t timestamp_us) {
    if (!optimizer || !optimizer->monitor) {
        return -1;
    }
    
    return network_monitor_record_packet_ack(optimizer->monitor, sequence, timestamp_us);
}

int network_optimizer_record_packet_lost(network_optimizer_t *optimizer, 
                                         uint32_t sequence) {
    if (!optimizer || !optimizer->monitor) {
        return -1;
    }
    
    return network_monitor_record_packet_lost(optimizer->monitor, sequence);
}

int network_optimizer_tune_socket(network_optimizer_t *optimizer, 
                                  int socket, 
                                  bool low_latency) {
    if (!optimizer || !optimizer->socket_tuning) {
        return -1;
    }
    
    if (low_latency) {
        return socket_tuning_tune_low_latency(optimizer->socket_tuning, socket);
    } else {
        return socket_tuning_tune_throughput(optimizer->socket_tuning, socket);
    }
}

char* network_optimizer_get_diagnostics_json(network_optimizer_t *optimizer) {
    if (!optimizer) {
        return NULL;
    }
    
    pthread_mutex_lock(&optimizer->lock);
    
    network_conditions_t conditions = network_monitor_get_conditions(optimizer->monitor);
    uint32_t bitrate = abr_controller_get_current_bitrate(optimizer->abr);
    uint32_t bw_estimate = bandwidth_estimator_get_estimated_bandwidth_mbps(optimizer->bandwidth_est);
    
    /* Allocate buffer for JSON */
    char *json = malloc(1024);
    if (!json) {
        pthread_mutex_unlock(&optimizer->lock);
        return NULL;
    }
    
    snprintf(json, 1024,
        "{\n"
        "  \"network\": {\n"
        "    \"rtt_ms\": %u,\n"
        "    \"jitter_ms\": %u,\n"
        "    \"packet_loss_percent\": %.2f,\n"
        "    \"bandwidth_mbps\": %u,\n"
        "    \"congestion_level\": %d\n"
        "  },\n"
        "  \"bitrate\": {\n"
        "    \"current_kbps\": %u,\n"
        "    \"estimated_bw_mbps\": %u\n"
        "  },\n"
        "  \"statistics\": {\n"
        "    \"optimizations\": %lu\n"
        "  }\n"
        "}",
        conditions.rtt_ms,
        conditions.rtt_variance_ms,
        conditions.packet_loss_percent,
        conditions.bandwidth_mbps,
        conditions.congestion_level,
        bitrate,
        bw_estimate,
        (unsigned long)optimizer->optimization_count
    );
    
    pthread_mutex_unlock(&optimizer->lock);
    return json;
}
