/*
 * network_config.h - Network configuration management
 */

#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Network configuration */
typedef struct {
    /* ABR settings */
    uint32_t min_bitrate_kbps;
    uint32_t max_bitrate_kbps;
    float switch_up_threshold;
    float switch_down_threshold;
    
    /* QoS settings */
    bool enable_qos;
    uint8_t video_dscp;
    uint8_t audio_dscp;
    
    /* Loss recovery */
    bool enable_fec;
    uint8_t fec_redundancy_percent;
    
    /* Buffer settings */
    uint32_t jitter_buffer_target_ms;
    uint32_t jitter_buffer_max_ms;
    
    /* Socket tuning */
    bool tune_socket;
    uint32_t socket_send_buf_kb;
    uint32_t socket_recv_buf_kb;
    bool enable_ecn;
} network_config_t;

/* Network config handle */
typedef struct network_config_manager network_config_manager_t;

/* Create network config manager */
network_config_manager_t* network_config_create(void);

/* Destroy network config manager */
void network_config_destroy(network_config_manager_t *config);

/* Load configuration from file */
int network_config_load(network_config_manager_t *manager, const char *config_file);

/* Save configuration to file */
int network_config_save(network_config_manager_t *manager, const char *config_file);

/* Get configuration */
network_config_t network_config_get(network_config_manager_t *manager);

/* Set configuration */
int network_config_set(network_config_manager_t *manager, const network_config_t *config);

/* Get default configuration */
network_config_t network_config_get_default(void);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_CONFIG_H */
