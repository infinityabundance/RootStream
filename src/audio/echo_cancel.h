/*
 * echo_cancel.h — Acoustic echo cancellation (AEC)
 *
 * Implements a block-based normalized least-mean-squares (NLMS) adaptive
 * filter that estimates and cancels acoustic echo from the far-end
 * reference signal.
 *
 * Usage
 * ─────
 *   1. Create an AEC state with aec_create()
 *   2. For every audio frame, call aec_process() with the capture buffer
 *      (microphone) and the reference buffer (speaker/far-end playback)
 *   3. aec_process() writes the echo-cancelled signal into @out_samples
 *
 * Integrates with audio_pipeline_t via aec_make_node().
 * When used in a pipeline the far-end reference must be set externally
 * via aec_set_reference() before each call to audio_pipeline_process().
 */

#ifndef ROOTSTREAM_ECHO_CANCEL_H
#define ROOTSTREAM_ECHO_CANCEL_H

#include "audio_pipeline.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** AEC configuration */
typedef struct {
    int   sample_rate;      /**< Sample rate in Hz (e.g. 48000) */
    int   channels;         /**< Mono (1) or stereo (2) */
    int   filter_length_ms; /**< Adaptive filter length in ms (e.g. 100) */
    float step_size;        /**< NLMS step size 0 < μ ≤ 1 (e.g. 0.5) */
} aec_config_t;

/** Opaque AEC state */
typedef struct aec_state_s aec_state_t;

/**
 * aec_create — allocate and initialise AEC state
 *
 * @param config  AEC configuration
 * @return        Non-NULL state, or NULL on failure
 */
aec_state_t *aec_create(const aec_config_t *config);

/**
 * aec_destroy — free AEC state
 *
 * @param state  State returned by aec_create()
 */
void aec_destroy(aec_state_t *state);

/**
 * aec_process — cancel echo from @mic_samples using @ref_samples
 *
 * @param state        AEC state
 * @param mic_samples  Input: microphone capture (float, interleaved)
 * @param ref_samples  Input: far-end reference (same layout as mic)
 * @param out_samples  Output: echo-cancelled signal (may alias mic_samples)
 * @param frame_count  Frames to process
 */
void aec_process(aec_state_t  *state,
                 const float  *mic_samples,
                 const float  *ref_samples,
                 float        *out_samples,
                 size_t        frame_count);

/**
 * aec_set_reference — store far-end reference for next pipeline call
 *
 * When the AEC is embedded in an audio_pipeline, call this before
 * audio_pipeline_process() to supply the current far-end reference buffer.
 *
 * @param state        AEC state
 * @param ref_samples  Reference buffer (must remain valid until after
 *                     audio_pipeline_process() returns)
 * @param frame_count  Number of frames in the buffer
 */
void aec_set_reference(aec_state_t *state,
                       const float *ref_samples,
                       size_t       frame_count);

/**
 * aec_make_node — return a pipeline node backed by @state
 *
 * The node reads the reference set via aec_set_reference() and
 * overwrites the pipeline buffer with the echo-cancelled signal.
 *
 * @param state  AEC state
 * @return       Initialised filter node
 */
audio_filter_node_t aec_make_node(aec_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_ECHO_CANCEL_H */
