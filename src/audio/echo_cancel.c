/*
 * echo_cancel.c — NLMS-based acoustic echo cancellation
 */

#include "echo_cancel.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

struct aec_state_s {
    int      sample_rate;
    int      channels;
    int      filter_taps;   /* filter_length_ms * sample_rate / 1000 */
    float    mu;            /* NLMS step size */

    /* Adaptive filter weights (length = filter_taps * channels) */
    float   *weights;

    /* Reference signal delay line (circular buffer) */
    float   *delay_line;
    int      delay_pos;

    /* Current reference buffer set by aec_set_reference() */
    const float *ref_buf;
    size_t       ref_len;
};

aec_state_t *aec_create(const aec_config_t *config) {
    if (!config || config->sample_rate <= 0 || config->channels <= 0) {
        return NULL;
    }

    int taps = (config->filter_length_ms * config->sample_rate) / 1000;
    if (taps <= 0) taps = 256;

    aec_state_t *s = calloc(1, sizeof(*s));
    if (!s) return NULL;

    s->sample_rate  = config->sample_rate;
    s->channels     = config->channels;
    s->filter_taps  = taps;
    s->mu           = (config->step_size > 0.0f && config->step_size <= 1.0f)
                          ? config->step_size : 0.5f;

    s->weights    = calloc((size_t)taps * (size_t)config->channels,
                           sizeof(float));
    s->delay_line = calloc((size_t)taps * (size_t)config->channels,
                           sizeof(float));

    if (!s->weights || !s->delay_line) {
        free(s->weights);
        free(s->delay_line);
        free(s);
        return NULL;
    }

    return s;
}

void aec_destroy(aec_state_t *state) {
    if (!state) return;
    free(state->weights);
    free(state->delay_line);
    free(state);
}

void aec_process(aec_state_t  *state,
                 const float  *mic_samples,
                 const float  *ref_samples,
                 float        *out_samples,
                 size_t        frame_count) {
    if (!state || !mic_samples || !ref_samples || !out_samples) return;

    int   taps = state->filter_taps;
    float mu   = state->mu;
    int   ch   = state->channels;

    for (size_t f = 0; f < frame_count; f++) {
        for (int c = 0; c < ch; c++) {
            /* Insert reference sample into delay line */
            state->delay_line[state->delay_pos * ch + c] =
                ref_samples[f * (size_t)ch + (size_t)c];
        }

        /* Compute echo estimate via dot product */
        float echo_est[8] = {0.0f};  /* max 8 channels */
        for (int k = 0; k < taps; k++) {
            int idx = ((state->delay_pos - k + taps) % taps) * ch;
            for (int c = 0; c < ch && c < 8; c++) {
                echo_est[c] += state->weights[k * ch + c]
                               * state->delay_line[idx + c];
            }
        }

        /* Error = mic - echo estimate */
        float power = 0.0f;
        for (int k = 0; k < taps; k++) {
            int idx = ((state->delay_pos - k + taps) % taps) * ch;
            for (int c = 0; c < ch && c < 8; c++) {
                power += state->delay_line[idx + c] *
                         state->delay_line[idx + c];
            }
        }
        float norm = (power > 1e-10f) ? mu / power : 0.0f;

        for (int c = 0; c < ch && c < 8; c++) {
            float err = mic_samples[f * (size_t)ch + (size_t)c] - echo_est[c];
            out_samples[f * (size_t)ch + (size_t)c] = err;

            /* NLMS weight update */
            for (int k = 0; k < taps; k++) {
                int idx = ((state->delay_pos - k + taps) % taps) * ch;
                state->weights[k * ch + c] +=
                    norm * err * state->delay_line[idx + c];
            }
        }

        state->delay_pos = (state->delay_pos + 1) % taps;
    }
}

void aec_set_reference(aec_state_t *state,
                       const float *ref_samples,
                       size_t       frame_count) {
    if (!state) return;
    state->ref_buf = ref_samples;
    state->ref_len = frame_count;
}

static void aec_pipeline_process(float *samples, size_t frame_count,
                                  int channels, void *user_data) {
    aec_state_t *s = (aec_state_t *)user_data;
    if (!s || !s->ref_buf) return;

    /* Use stack buffer for output to avoid aliasing issues */
    float *tmp = malloc(frame_count * (size_t)channels * sizeof(float));
    if (!tmp) return;

    size_t ref_frames = (s->ref_len < frame_count) ? s->ref_len : frame_count;
    aec_process(s, samples, s->ref_buf, tmp, ref_frames);
    memcpy(samples, tmp, ref_frames * (size_t)channels * sizeof(float));
    free(tmp);

    s->ref_buf = NULL;
    s->ref_len = 0;
}

audio_filter_node_t aec_make_node(aec_state_t *state) {
    audio_filter_node_t node;
    memset(&node, 0, sizeof(node));
    node.name      = "aec";
    node.process   = aec_pipeline_process;
    node.user_data = state;
    node.enabled   = true;
    return node;
}
