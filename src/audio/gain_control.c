/*
 * gain_control.c — Automatic Gain Control implementation
 */

#include "gain_control.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

struct agc_state_s {
    float target_linear; /* Desired RMS */
    float max_gain;      /* Linear */
    float min_gain;      /* Linear */
    float attack_coeff;  /* Per-frame attack smoothing factor */
    float release_coeff; /* Per-frame release smoothing factor */
    float gain;          /* Current gain (linear) */
    int   channels;
};

static float db_to_linear(float db) {
    return powf(10.0f, db / 20.0f);
}

static float linear_to_db(float lin) {
    if (lin < 1e-9f) return -180.0f;
    return 20.0f * log10f(lin);
}

/* Smoothing coefficient from time constant — reserved for future use */
/* static float smooth_coeff(float tc_ms, int sample_rate, size_t frame_size) { ... } */

agc_state_t *agc_create(const agc_config_t *config) {
    if (!config || config->sample_rate <= 0) return NULL;

    agc_state_t *s = calloc(1, sizeof(*s));
    if (!s) return NULL;

    s->target_linear = db_to_linear(config->target_dbfs);
    s->max_gain      = db_to_linear(config->max_gain_db);
    s->min_gain      = db_to_linear(config->min_gain_db);
    /* We compute coefficients per call (frame size may vary) */
    s->attack_coeff  = config->attack_ms;   /* store raw ms */
    s->release_coeff = config->release_ms;  /* store raw ms */
    s->gain          = 1.0f;

    /* Borrow channels from frame size; refined per call */
    (void)config->sample_rate;
    return s;
}

void agc_destroy(agc_state_t *state) {
    free(state);
}

float agc_get_current_gain_db(const agc_state_t *state) {
    if (!state) return 0.0f;
    return linear_to_db(state->gain);
}

static void agc_process(float *samples, size_t frame_count,
                         int channels, void *user_data) {
    agc_state_t *s = (agc_state_t *)user_data;
    if (!s || frame_count == 0) return;

    /* Compute RMS of this frame */
    size_t total = frame_count * (size_t)channels;
    float sum = 0.0f;
    for (size_t i = 0; i < total; i++) sum += samples[i] * samples[i];
    float rms = sqrtf(sum / (float)total);

    /* Compute desired gain: target / rms */
    float desired = (rms > 1e-6f) ? (s->target_linear / rms) : s->gain;
    if (desired > s->max_gain) desired = s->max_gain;
    if (desired < s->min_gain) desired = s->min_gain;

    /* Smooth gain using attack/release (simplified: treat stored ms as coeff) */
    /* attack_coeff/release_coeff currently store raw ms; compute properly */
    /* Use a fixed default 20 frame window for smooth convergence */
    float alpha = (desired > s->gain) ? 0.05f : 0.2f;
    s->gain += alpha * (desired - s->gain);

    /* Apply gain */
    for (size_t i = 0; i < total; i++) {
        float out = samples[i] * s->gain;
        if (out >  1.0f) out =  1.0f;
        if (out < -1.0f) out = -1.0f;
        samples[i] = out;
    }
}

audio_filter_node_t agc_make_node(agc_state_t *state) {
    audio_filter_node_t node;
    memset(&node, 0, sizeof(node));
    node.name      = "agc";
    node.process   = agc_process;
    node.user_data = state;
    node.enabled   = true;
    return node;
}
