/*
 * adaptive_bitrate.c - Adaptive Bitrate Controller implementation
 */

#include "adaptive_bitrate.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define MAX_PROFILES 10
#define DEFAULT_PROFILE_HOLD_TIME_MS 5000  /* Min time before switching again */

struct adaptive_bitrate_controller {
    network_monitor_t *network_monitor;
    
    bitrate_profile_t profiles[MAX_PROFILES];
    uint32_t profile_count;
    uint32_t current_profile_index;
    
    abr_config_t config;
    
    uint64_t last_profile_switch_us;
    uint32_t profile_hold_time_ms;
    uint32_t profile_switches;
    
    pthread_mutex_t lock;
};

/* Get current time in microseconds */
static uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

abr_controller_t* abr_controller_create(network_monitor_t *monitor) {
    if (!monitor) {
        return NULL;
    }
    
    abr_controller_t *controller = calloc(1, sizeof(abr_controller_t));
    if (!controller) {
        return NULL;
    }
    
    controller->network_monitor = monitor;
    pthread_mutex_init(&controller->lock, NULL);
    
    /* Set default configuration */
    controller->config.min_bitrate_kbps = 500;
    controller->config.max_bitrate_kbps = 50000;
    controller->config.startup_bitrate_kbps = 5000;
    controller->config.buffer_target_ms = 100;
    controller->config.switch_up_threshold = 0.8f;    /* 80% of max bandwidth */
    controller->config.switch_down_threshold = 1.2f;  /* 120% of available bandwidth */
    
    controller->profile_hold_time_ms = DEFAULT_PROFILE_HOLD_TIME_MS;
    controller->last_profile_switch_us = get_time_us();
    
    return controller;
}

void abr_controller_destroy(abr_controller_t *controller) {
    if (!controller) {
        return;
    }
    
    pthread_mutex_destroy(&controller->lock);
    free(controller);
}

int abr_controller_configure(abr_controller_t *controller, const abr_config_t *config) {
    if (!controller || !config) {
        return -1;
    }
    
    pthread_mutex_lock(&controller->lock);
    controller->config = *config;
    pthread_mutex_unlock(&controller->lock);
    
    return 0;
}

int abr_controller_add_profile(abr_controller_t *controller, 
                               uint32_t bitrate_kbps, 
                               uint32_t width, 
                               uint32_t height, 
                               uint32_t fps,
                               const char *codec, 
                               const char *preset) {
    if (!controller || controller->profile_count >= MAX_PROFILES) {
        return -1;
    }
    
    pthread_mutex_lock(&controller->lock);
    
    bitrate_profile_t *profile = &controller->profiles[controller->profile_count];
    profile->bitrate_kbps = bitrate_kbps;
    profile->width = width;
    profile->height = height;
    profile->fps = fps;
    profile->codec = codec;
    profile->preset = preset;
    
    controller->profile_count++;
    
    /* Sort profiles by bitrate (insertion sort - simple for small arrays) */
    for (uint32_t i = controller->profile_count - 1; i > 0; i--) {
        if (controller->profiles[i].bitrate_kbps < controller->profiles[i-1].bitrate_kbps) {
            bitrate_profile_t tmp = controller->profiles[i];
            controller->profiles[i] = controller->profiles[i-1];
            controller->profiles[i-1] = tmp;
        } else {
            break;
        }
    }
    
    pthread_mutex_unlock(&controller->lock);
    return 0;
}

const bitrate_profile_t* abr_controller_get_recommended_profile(abr_controller_t *controller) {
    if (!controller || controller->profile_count == 0) {
        return NULL;
    }
    
    pthread_mutex_lock(&controller->lock);
    
    /* Get current network conditions */
    network_conditions_t conditions = network_monitor_get_conditions(controller->network_monitor);
    
    /* Check if we should switch profiles */
    uint64_t now = get_time_us();
    uint64_t time_since_switch = now - controller->last_profile_switch_us;
    
    /* Don't switch too frequently */
    if (time_since_switch < (uint64_t)controller->profile_hold_time_ms * 1000) {
        const bitrate_profile_t *current = &controller->profiles[controller->current_profile_index];
        pthread_mutex_unlock(&controller->lock);
        return current;
    }
    
    /* Calculate available bandwidth in kbps */
    uint32_t available_kbps = conditions.bandwidth_mbps * 1000;
    
    /* Find appropriate profile based on available bandwidth */
    uint32_t target_index = controller->current_profile_index;
    
    /* Check if we should upgrade */
    if (controller->current_profile_index < controller->profile_count - 1) {
        const bitrate_profile_t *next = &controller->profiles[controller->current_profile_index + 1];
        if (next->bitrate_kbps < available_kbps * controller->config.switch_up_threshold) {
            /* We have enough bandwidth to upgrade */
            if (conditions.congestion_level <= CONGESTION_GOOD) {
                target_index = controller->current_profile_index + 1;
            }
        }
    }
    
    /* Check if we should downgrade */
    if (controller->current_profile_index > 0) {
        const bitrate_profile_t *current = &controller->profiles[controller->current_profile_index];
        if (current->bitrate_kbps > available_kbps * controller->config.switch_down_threshold ||
            conditions.congestion_level >= CONGESTION_POOR) {
            /* Not enough bandwidth or too much congestion, downgrade */
            target_index = controller->current_profile_index - 1;
        }
    }
    
    /* Update current profile if changed */
    if (target_index != controller->current_profile_index) {
        controller->current_profile_index = target_index;
        controller->last_profile_switch_us = now;
        controller->profile_switches++;
    }
    
    const bitrate_profile_t *recommended = &controller->profiles[controller->current_profile_index];
    
    pthread_mutex_unlock(&controller->lock);
    return recommended;
}

uint32_t abr_controller_predict_next_bitrate(abr_controller_t *controller) {
    const bitrate_profile_t *profile = abr_controller_get_recommended_profile(controller);
    return profile ? profile->bitrate_kbps : 0;
}

int abr_controller_set_target_bitrate(abr_controller_t *controller, 
                                      uint32_t bitrate_kbps) {
    if (!controller || controller->profile_count == 0) {
        return -1;
    }
    
    pthread_mutex_lock(&controller->lock);
    
    /* Find closest profile to target bitrate */
    uint32_t closest_index = 0;
    uint32_t min_diff = UINT32_MAX;
    
    for (uint32_t i = 0; i < controller->profile_count; i++) {
        uint32_t diff = abs((int32_t)controller->profiles[i].bitrate_kbps - (int32_t)bitrate_kbps);
        if (diff < min_diff) {
            min_diff = diff;
            closest_index = i;
        }
    }
    
    controller->current_profile_index = closest_index;
    controller->last_profile_switch_us = get_time_us();
    controller->profile_switches++;
    
    pthread_mutex_unlock(&controller->lock);
    return 0;
}

uint32_t abr_controller_get_current_bitrate(abr_controller_t *controller) {
    if (!controller || controller->profile_count == 0) {
        return 0;
    }
    
    pthread_mutex_lock(&controller->lock);
    uint32_t bitrate = controller->profiles[controller->current_profile_index].bitrate_kbps;
    pthread_mutex_unlock(&controller->lock);
    
    return bitrate;
}

uint32_t abr_controller_get_profile_switches(abr_controller_t *controller) {
    if (!controller) {
        return 0;
    }
    
    pthread_mutex_lock(&controller->lock);
    uint32_t switches = controller->profile_switches;
    pthread_mutex_unlock(&controller->lock);
    
    return switches;
}

uint64_t abr_controller_get_time_in_current_profile(abr_controller_t *controller) {
    if (!controller) {
        return 0;
    }
    
    pthread_mutex_lock(&controller->lock);
    uint64_t time_in_profile = (get_time_us() - controller->last_profile_switch_us) / 1000;
    pthread_mutex_unlock(&controller->lock);
    
    return time_in_profile;
}
