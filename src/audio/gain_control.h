/*
 * gain_control.h — Automatic Gain Control (AGC)
 *
 * Implements a feed-forward AGC that keeps the output RMS within a
 * configurable target window using a smoothed gain envelope.
 *
 * Integrates with audio_pipeline_t as a filter node.
 */

#ifndef ROOTSTREAM_GAIN_CONTROL_H
#define ROOTSTREAM_GAIN_CONTROL_H

#include "audio_pipeline.h"

#ifdef __cplusplus
extern "C" {
#endif

/** AGC configuration */
typedef struct {
    float target_dbfs; /**< Desired output RMS level (e.g. -18.0) */
    float max_gain_db; /**< Maximum gain to apply (e.g. +30.0) */
    float min_gain_db; /**< Minimum gain to apply (e.g. -20.0) */
    float attack_ms;   /**< Gain increase time constant in ms */
    float release_ms;  /**< Gain decrease time constant in ms */
    int sample_rate;   /**< Sample rate in Hz */
} agc_config_t;

/** Opaque AGC state */
typedef struct agc_state_s agc_state_t;

/**
 * agc_create — allocate and initialise AGC state
 *
 * @param config  AGC configuration
 * @return        Non-NULL state, or NULL on failure
 */
agc_state_t *agc_create(const agc_config_t *config);

/**
 * agc_destroy — free AGC state
 *
 * @param state  State returned by agc_create()
 */
void agc_destroy(agc_state_t *state);

/**
 * agc_get_current_gain_db — return the current gain envelope value
 *
 * Useful for UI metering.
 *
 * @param state  AGC state
 * @return       Current gain in dB
 */
float agc_get_current_gain_db(const agc_state_t *state);

/**
 * agc_make_node — return a pipeline node backed by @state
 *
 * @param state  AGC state
 * @return       Initialised filter node
 */
audio_filter_node_t agc_make_node(agc_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_GAIN_CONTROL_H */
