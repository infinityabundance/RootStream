/*
 * adaptive_bitrate.h - Adaptive Bitrate Controller
 * 
 * Dynamically adjusts video bitrate, resolution, and codec based on network conditions
 */

#ifndef ADAPTIVE_BITRATE_H
#define ADAPTIVE_BITRATE_H

#include "network_monitor.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Bitrate profile */
typedef struct {
    uint32_t bitrate_kbps;
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    const char *codec;      /* H.264, VP9, AV1 */
    const char *preset;     /* fast, medium, slow */
} bitrate_profile_t;

/* ABR configuration */
typedef struct {
    uint32_t min_bitrate_kbps;
    uint32_t max_bitrate_kbps;
    uint32_t startup_bitrate_kbps;
    int32_t buffer_target_ms;       /* Jitter buffer target */
    float switch_up_threshold;       /* % of max bandwidth to trigger upgrade */
    float switch_down_threshold;     /* % to trigger downgrade */
} abr_config_t;

/* Adaptive bitrate controller handle */
typedef struct adaptive_bitrate_controller abr_controller_t;

/* Create ABR controller */
abr_controller_t* abr_controller_create(network_monitor_t *monitor);

/* Destroy ABR controller */
void abr_controller_destroy(abr_controller_t *controller);

/* Configure ABR controller */
int abr_controller_configure(abr_controller_t *controller, const abr_config_t *config);

/* Add bitrate profile */
int abr_controller_add_profile(abr_controller_t *controller, 
                               uint32_t bitrate_kbps, 
                               uint32_t width, 
                               uint32_t height, 
                               uint32_t fps,
                               const char *codec, 
                               const char *preset);

/* Get recommended profile based on current network conditions */
const bitrate_profile_t* abr_controller_get_recommended_profile(abr_controller_t *controller);

/* Predict next bitrate */
uint32_t abr_controller_predict_next_bitrate(abr_controller_t *controller);

/* Manually set target bitrate */
int abr_controller_set_target_bitrate(abr_controller_t *controller, 
                                      uint32_t bitrate_kbps);

/* Get current bitrate */
uint32_t abr_controller_get_current_bitrate(abr_controller_t *controller);

/* Statistics */
uint32_t abr_controller_get_profile_switches(abr_controller_t *controller);
uint64_t abr_controller_get_time_in_current_profile(abr_controller_t *controller);

#ifdef __cplusplus
}
#endif

#endif /* ADAPTIVE_BITRATE_H */
