/*
 * noise_filter.c — Noise gate and spectral subtraction filter implementation
 */

#include "noise_filter.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ── Noise gate ────────────────────────────────────────────────── */

struct noise_gate_state_s {
    float threshold_linear; /* Converted from dBFS */
    int hold_samples;       /* Release time in samples */
    int hold_counter;       /* Samples remaining in hold */
    bool gate_open;
};

/* dBFS to linear amplitude */
static float dbfs_to_linear(float dbfs) {
    return powf(10.0f, dbfs / 20.0f);
}

noise_gate_state_t *noise_gate_create(const noise_gate_config_t *config) {
    if (!config || config->sample_rate <= 0)
        return NULL;

    noise_gate_state_t *s = calloc(1, sizeof(*s));
    if (!s)
        return NULL;

    s->threshold_linear = dbfs_to_linear(config->threshold_dbfs);
    s->hold_samples = (int)(config->release_ms * 0.001f * (float)config->sample_rate);
    if (s->hold_samples < 1)
        s->hold_samples = 1;
    s->gate_open = false;
    return s;
}

void noise_gate_destroy(noise_gate_state_t *state) {
    free(state);
}

static float rms_level(const float *samples, size_t n) {
    if (n == 0)
        return 0.0f;
    float sum = 0.0f;
    for (size_t i = 0; i < n; i++) sum += samples[i] * samples[i];
    return sqrtf(sum / (float)n);
}

static void noise_gate_process(float *samples, size_t frame_count, int channels, void *user_data) {
    noise_gate_state_t *s = (noise_gate_state_t *)user_data;
    if (!s)
        return;

    size_t total = frame_count * (size_t)channels;
    float level = rms_level(samples, total);

    if (level >= s->threshold_linear) {
        s->gate_open = true;
        s->hold_counter = s->hold_samples;
    } else if (s->gate_open) {
        s->hold_counter -= (int)frame_count;
        if (s->hold_counter <= 0) {
            s->gate_open = false;
        }
    }

    if (!s->gate_open) {
        memset(samples, 0, total * sizeof(float));
    }
}

audio_filter_node_t noise_gate_make_node(noise_gate_state_t *state) {
    audio_filter_node_t node;
    memset(&node, 0, sizeof(node));
    node.name = "noise-gate";
    node.process = noise_gate_process;
    node.user_data = state;
    node.enabled = true;
    return node;
}

/* ── Spectral subtraction ──────────────────────────────────────── */

/* Simple single-band implementation: estimate noise floor during
 * quiet frames and subtract it from every frame's energy.         */

#define SPEC_SUB_HISTORY_LEN 32 /* frames used for noise floor estimate */

struct spectral_sub_state_s {
    float over_sub;
    float floor_linear;
    float history[SPEC_SUB_HISTORY_LEN];
    int history_head;
    int history_filled;
    float noise_estimate;
};

spectral_sub_state_t *spectral_sub_create(const spectral_sub_config_t *config) {
    if (!config || config->sample_rate <= 0)
        return NULL;

    spectral_sub_state_t *s = calloc(1, sizeof(*s));
    if (!s)
        return NULL;

    s->over_sub = (config->over_sub > 0.0f) ? config->over_sub : 1.5f;
    s->floor_linear = dbfs_to_linear(config->floor_db < 0.0f ? config->floor_db : -60.0f);
    s->noise_estimate = 0.0f;
    return s;
}

void spectral_sub_destroy(spectral_sub_state_t *state) {
    free(state);
}

static void spectral_sub_process(float *samples, size_t frame_count, int channels,
                                 void *user_data) {
    spectral_sub_state_t *s = (spectral_sub_state_t *)user_data;
    if (!s)
        return;

    size_t total = frame_count * (size_t)channels;
    float level = rms_level(samples, total);

    /* Update noise history */
    s->history[s->history_head] = level;
    s->history_head = (s->history_head + 1) % SPEC_SUB_HISTORY_LEN;
    if (s->history_filled < SPEC_SUB_HISTORY_LEN)
        s->history_filled++;

    /* Compute noise floor estimate (minimum of history) */
    float min_level = s->history[0];
    for (int i = 1; i < s->history_filled; i++) {
        if (s->history[i] < min_level)
            min_level = s->history[i];
    }
    s->noise_estimate = min_level;

    /* Subtract: scale factor max(1 - alpha*noise/level, floor) */
    float target_rms = level - s->over_sub * s->noise_estimate;
    if (target_rms < s->floor_linear)
        target_rms = s->floor_linear;

    float scale = (level > 1e-6f) ? (target_rms / level) : 1.0f;
    for (size_t i = 0; i < total; i++) {
        samples[i] *= scale;
    }
}

audio_filter_node_t spectral_sub_make_node(spectral_sub_state_t *state) {
    audio_filter_node_t node;
    memset(&node, 0, sizeof(node));
    node.name = "spectral-sub";
    node.process = spectral_sub_process;
    node.user_data = state;
    node.enabled = true;
    return node;
}
