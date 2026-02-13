/*
 * network_config.c - Network configuration management implementation
 */

#include "network_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

struct network_config_manager {
    network_config_t config;
    pthread_mutex_t lock;
};

network_config_manager_t* network_config_create(void) {
    network_config_manager_t *manager = calloc(1, sizeof(network_config_manager_t));
    if (!manager) {
        return NULL;
    }
    
    pthread_mutex_init(&manager->lock, NULL);
    
    /* Set defaults */
    manager->config = network_config_get_default();
    
    return manager;
}

void network_config_destroy(network_config_manager_t *manager) {
    if (!manager) {
        return;
    }
    
    pthread_mutex_destroy(&manager->lock);
    free(manager);
}

int network_config_load(network_config_manager_t *manager, const char *config_file) {
    if (!manager || !config_file) {
        return -1;
    }
    
    /* Simple file-based configuration loading */
    FILE *fp = fopen(config_file, "r");
    if (!fp) {
        return -1;
    }
    
    pthread_mutex_lock(&manager->lock);
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char key[128], value[128];
        if (sscanf(line, "%127[^=]=%127s", key, value) == 2) {
            /* Parse configuration values */
            if (strcmp(key, "min_bitrate_kbps") == 0) {
                manager->config.min_bitrate_kbps = atoi(value);
            } else if (strcmp(key, "max_bitrate_kbps") == 0) {
                manager->config.max_bitrate_kbps = atoi(value);
            } else if (strcmp(key, "enable_qos") == 0) {
                manager->config.enable_qos = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "enable_fec") == 0) {
                manager->config.enable_fec = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "jitter_buffer_target_ms") == 0) {
                manager->config.jitter_buffer_target_ms = atoi(value);
            }
            /* Add more configuration options as needed */
        }
    }
    
    pthread_mutex_unlock(&manager->lock);
    fclose(fp);
    
    return 0;
}

int network_config_save(network_config_manager_t *manager, const char *config_file) {
    if (!manager || !config_file) {
        return -1;
    }
    
    FILE *fp = fopen(config_file, "w");
    if (!fp) {
        return -1;
    }
    
    pthread_mutex_lock(&manager->lock);
    
    fprintf(fp, "# RootStream Network Configuration\n");
    fprintf(fp, "min_bitrate_kbps=%u\n", manager->config.min_bitrate_kbps);
    fprintf(fp, "max_bitrate_kbps=%u\n", manager->config.max_bitrate_kbps);
    fprintf(fp, "enable_qos=%s\n", manager->config.enable_qos ? "true" : "false");
    fprintf(fp, "enable_fec=%s\n", manager->config.enable_fec ? "true" : "false");
    fprintf(fp, "jitter_buffer_target_ms=%u\n", manager->config.jitter_buffer_target_ms);
    
    pthread_mutex_unlock(&manager->lock);
    fclose(fp);
    
    return 0;
}

network_config_t network_config_get(network_config_manager_t *manager) {
    network_config_t config;
    memset(&config, 0, sizeof(config));
    
    if (!manager) {
        return config;
    }
    
    pthread_mutex_lock(&manager->lock);
    config = manager->config;
    pthread_mutex_unlock(&manager->lock);
    
    return config;
}

int network_config_set(network_config_manager_t *manager, const network_config_t *config) {
    if (!manager || !config) {
        return -1;
    }
    
    pthread_mutex_lock(&manager->lock);
    manager->config = *config;
    pthread_mutex_unlock(&manager->lock);
    
    return 0;
}

network_config_t network_config_get_default(void) {
    network_config_t config;
    
    /* ABR settings */
    config.min_bitrate_kbps = 500;
    config.max_bitrate_kbps = 50000;
    config.switch_up_threshold = 0.8f;
    config.switch_down_threshold = 1.2f;
    
    /* QoS settings */
    config.enable_qos = true;
    config.video_dscp = 46;  /* EF */
    config.audio_dscp = 26;  /* AF31 */
    
    /* Loss recovery */
    config.enable_fec = true;
    config.fec_redundancy_percent = 10;
    
    /* Buffer settings */
    config.jitter_buffer_target_ms = 100;
    config.jitter_buffer_max_ms = 300;
    
    /* Socket tuning */
    config.tune_socket = true;
    config.socket_send_buf_kb = 256;
    config.socket_recv_buf_kb = 256;
    config.enable_ecn = true;
    
    return config;
}
