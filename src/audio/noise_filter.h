/*
 * noise_filter.h — Noise gate and spectral subtraction filter
 *
 * Provides two complementary filters:
 *   1. Noise gate   — silences samples below an RMS energy threshold
 *   2. Spectral sub — estimates a noise floor during silence and
 *                     subtracts it from active audio (band-level)
 *
 * Both filters integrate with audio_pipeline_t as filter nodes.
 */

#ifndef ROOTSTREAM_NOISE_FILTER_H
#define ROOTSTREAM_NOISE_FILTER_H

#include <stdint.h>

#include "audio_pipeline.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Noise gate ────────────────────────────────────────────────── */

/** Configuration for the noise gate */
typedef struct {
    float threshold_dbfs; /**< Gate opens above this level (e.g. -40.0) */
    float release_ms;     /**< Hold-open time after level drops (ms) */
    int sample_rate;      /**< Sample rate in Hz */
} noise_gate_config_t;

/** Opaque noise gate state */
typedef struct noise_gate_state_s noise_gate_state_t;

/**
 * noise_gate_create — allocate and initialise gate state
 *
 * @param config  Gate configuration
 * @return        Non-NULL state, or NULL on OOM / bad config
 */
noise_gate_state_t *noise_gate_create(const noise_gate_config_t *config);

/**
 * noise_gate_destroy — free gate state
 *
 * @param state  State returned by noise_gate_create()
 */
void noise_gate_destroy(noise_gate_state_t *state);

/**
 * noise_gate_make_node — return a pipeline node backed by @state
 *
 * The returned node's user_data is @state; the caller keeps ownership.
 *
 * @param state  Noise gate state
 * @return       Initialised filter node
 */
audio_filter_node_t noise_gate_make_node(noise_gate_state_t *state);

/* ── Spectral subtraction filter ───────────────────────────────── */

/** Configuration for the spectral subtraction filter */
typedef struct {
    int sample_rate; /**< Sample rate in Hz */
    int channels;    /**< Channel count */
    float over_sub;  /**< Over-subtraction factor (typically 1.5–2.0) */
    float floor_db;  /**< Noise floor lower bound (dBFS, e.g. -60.0) */
} spectral_sub_config_t;

/** Opaque spectral subtraction state */
typedef struct spectral_sub_state_s spectral_sub_state_t;

/**
 * spectral_sub_create — allocate spectral subtraction state
 *
 * @param config  Filter configuration
 * @return        Non-NULL state, or NULL on failure
 */
spectral_sub_state_t *spectral_sub_create(const spectral_sub_config_t *config);

/**
 * spectral_sub_destroy — free spectral subtraction state
 *
 * @param state  State returned by spectral_sub_create()
 */
void spectral_sub_destroy(spectral_sub_state_t *state);

/**
 * spectral_sub_make_node — return a pipeline node backed by @state
 *
 * @param state  Spectral subtraction state
 * @return       Initialised filter node
 */
audio_filter_node_t spectral_sub_make_node(spectral_sub_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_NOISE_FILTER_H */
