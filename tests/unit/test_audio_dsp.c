/*
 * test_audio_dsp.c — Unit tests for PHASE-36 Audio DSP Pipeline
 *
 * Tests audio_pipeline, noise_filter, gain_control, and echo_cancel
 * using synthetic float PCM buffers (no real audio hardware required).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/audio/audio_pipeline.h"
#include "../../src/audio/noise_filter.h"
#include "../../src/audio/gain_control.h"
#include "../../src/audio/echo_cancel.h"

/* ── Helpers ─────────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

#define SAMPLE_RATE   48000
#define FRAME_SIZE    512
#define CHANNELS      1

static void fill_sine(float *buf, size_t n, float freq, float amp) {
    for (size_t i = 0; i < n; i++) {
        buf[i] = amp * sinf(2.0f * 3.14159265f * freq * (float)i / SAMPLE_RATE);
    }
}

static float rms(const float *buf, size_t n) {
    float sum = 0.0f;
    for (size_t i = 0; i < n; i++) sum += buf[i] * buf[i];
    return sqrtf(sum / (float)n);
}

/* ── audio_pipeline tests ────────────────────────────────────────── */

static int test_pipeline_create_destroy(void) {
    printf("\n=== test_pipeline_create_destroy ===\n");

    audio_pipeline_t *p = audio_pipeline_create(SAMPLE_RATE, CHANNELS);
    TEST_ASSERT(p != NULL, "pipeline created");
    TEST_ASSERT(audio_pipeline_node_count(p) == 0, "initial node count 0");
    TEST_ASSERT(audio_pipeline_get_sample_rate(p) == SAMPLE_RATE, "sample rate");
    TEST_ASSERT(audio_pipeline_get_channels(p)    == CHANNELS,    "channels");

    audio_pipeline_destroy(p);
    TEST_PASS("pipeline create/destroy");
    return 0;
}

/* Static no-op filter used by test_pipeline_add_remove */
static void noop_filter(float *s, size_t fc, int ch, void *ud) {
    (void)s; (void)fc; (void)ch; (void)ud;
}

static int test_pipeline_add_remove(void) {
    printf("\n=== test_pipeline_add_remove ===\n");

    audio_pipeline_t *p = audio_pipeline_create(SAMPLE_RATE, CHANNELS);
    TEST_ASSERT(p != NULL, "pipeline created");

    audio_filter_node_t n1 = {
        .name      = "noop-a",
        .process   = noop_filter,
        .user_data = NULL,
        .enabled   = false,
    };

    int rc = audio_pipeline_add_node(p, &n1);
    TEST_ASSERT(rc == 0, "add node returns 0");
    TEST_ASSERT(audio_pipeline_node_count(p) == 1, "node count == 1");

    rc = audio_pipeline_remove_node(p, "noop-a");
    TEST_ASSERT(rc == 0, "remove existing node returns 0");
    TEST_ASSERT(audio_pipeline_node_count(p) == 0, "node count back to 0");

    rc = audio_pipeline_remove_node(p, "nonexistent");
    TEST_ASSERT(rc == -1, "remove nonexistent returns -1");

    audio_pipeline_destroy(p);
    TEST_PASS("pipeline add/remove");
    return 0;
}

static int test_pipeline_process_passthrough(void) {
    printf("\n=== test_pipeline_process_passthrough ===\n");

    audio_pipeline_t *p = audio_pipeline_create(SAMPLE_RATE, CHANNELS);
    TEST_ASSERT(p != NULL, "pipeline created");

    float buf[FRAME_SIZE];
    fill_sine(buf, FRAME_SIZE, 440.0f, 0.5f);
    float original_rms = rms(buf, FRAME_SIZE);

    /* Empty pipeline — output should be identical to input */
    audio_pipeline_process(p, buf, FRAME_SIZE);

    float out_rms = rms(buf, FRAME_SIZE);
    float diff = fabsf(out_rms - original_rms);
    TEST_ASSERT(diff < 1e-6f, "empty pipeline preserves signal");

    audio_pipeline_destroy(p);
    TEST_PASS("pipeline passthrough with no nodes");
    return 0;
}

static int test_pipeline_null_guards(void) {
    printf("\n=== test_pipeline_null_guards ===\n");

    audio_pipeline_t *p = audio_pipeline_create(0, 1);
    TEST_ASSERT(p == NULL, "create with 0 sample_rate returns NULL");

    p = audio_pipeline_create(48000, 0);
    TEST_ASSERT(p == NULL, "create with 0 channels returns NULL");

    TEST_ASSERT(audio_pipeline_node_count(NULL) == 0,
                "node_count(NULL) == 0");
    TEST_ASSERT(audio_pipeline_get_sample_rate(NULL) == 0,
                "get_sample_rate(NULL) == 0");

    /* process with NULL pipeline must not crash */
    float buf[4] = {0};
    audio_pipeline_process(NULL, buf, 4);

    TEST_PASS("pipeline NULL guards");
    return 0;
}

/* ── Noise gate tests ────────────────────────────────────────────── */

static int test_noise_gate_silences_quiet(void) {
    printf("\n=== test_noise_gate_silences_quiet ===\n");

    noise_gate_config_t cfg = {
        .threshold_dbfs = -20.0f,
        .release_ms     = 0.0f,
        .sample_rate    = SAMPLE_RATE,
    };
    noise_gate_state_t *s = noise_gate_create(&cfg);
    TEST_ASSERT(s != NULL, "noise gate created");

    audio_pipeline_t *p = audio_pipeline_create(SAMPLE_RATE, CHANNELS);
    TEST_ASSERT(p != NULL, "pipeline created");

    audio_filter_node_t node = noise_gate_make_node(s);
    int rc = audio_pipeline_add_node(p, &node);
    TEST_ASSERT(rc == 0, "noise gate node added");

    /* Very quiet signal (-60 dBFS ≈ linear 0.001) */
    float buf[FRAME_SIZE];
    fill_sine(buf, FRAME_SIZE, 440.0f, 0.001f);

    audio_pipeline_process(p, buf, FRAME_SIZE);

    float out_rms_val = rms(buf, FRAME_SIZE);
    /* Should be silenced (gate closed) */
    TEST_ASSERT(out_rms_val < 1e-6f, "quiet signal silenced by gate");

    audio_pipeline_destroy(p);
    noise_gate_destroy(s);
    TEST_PASS("noise gate silences quiet signal");
    return 0;
}

static int test_noise_gate_passes_loud(void) {
    printf("\n=== test_noise_gate_passes_loud ===\n");

    noise_gate_config_t cfg = {
        .threshold_dbfs = -40.0f,
        .release_ms     = 10.0f,
        .sample_rate    = SAMPLE_RATE,
    };
    noise_gate_state_t *s = noise_gate_create(&cfg);
    TEST_ASSERT(s != NULL, "noise gate created");

    audio_pipeline_t *p = audio_pipeline_create(SAMPLE_RATE, CHANNELS);
    TEST_ASSERT(p != NULL, "pipeline created");

    audio_filter_node_t node = noise_gate_make_node(s);
    audio_pipeline_add_node(p, &node);

    /* Loud signal at 0.5 linear ≈ -6 dBFS */
    float buf[FRAME_SIZE];
    fill_sine(buf, FRAME_SIZE, 440.0f, 0.5f);
    float in_rms = rms(buf, FRAME_SIZE);

    audio_pipeline_process(p, buf, FRAME_SIZE);

    float out_rms_val = rms(buf, FRAME_SIZE);
    /* Gate should be open; signal passes through */
    TEST_ASSERT(out_rms_val > in_rms * 0.9f, "loud signal passes gate");

    audio_pipeline_destroy(p);
    noise_gate_destroy(s);
    TEST_PASS("noise gate passes loud signal");
    return 0;
}

static int test_noise_gate_null(void) {
    printf("\n=== test_noise_gate_null ===\n");

    noise_gate_state_t *s = noise_gate_create(NULL);
    TEST_ASSERT(s == NULL, "create NULL config returns NULL");

    noise_gate_destroy(NULL);  /* must not crash */
    TEST_PASS("noise gate NULL guards");
    return 0;
}

/* ── AGC tests ───────────────────────────────────────────────────── */

static int test_agc_boosts_quiet(void) {
    printf("\n=== test_agc_boosts_quiet ===\n");

    agc_config_t cfg = {
        .target_dbfs  = -18.0f,
        .max_gain_db  = +30.0f,
        .min_gain_db  = -20.0f,
        .attack_ms    = 10.0f,
        .release_ms   = 50.0f,
        .sample_rate  = SAMPLE_RATE,
    };
    agc_state_t *s = agc_create(&cfg);
    TEST_ASSERT(s != NULL, "AGC created");

    audio_pipeline_t *p = audio_pipeline_create(SAMPLE_RATE, CHANNELS);
    TEST_ASSERT(p != NULL, "pipeline created");

    audio_filter_node_t node = agc_make_node(s);
    audio_pipeline_add_node(p, &node);

    /* Quiet signal at -60 dBFS */
    float buf[FRAME_SIZE];
    fill_sine(buf, FRAME_SIZE, 440.0f, 0.001f);
    float in_rms_val = rms(buf, FRAME_SIZE);

    /* Run several frames to let the gain converge */
    for (int i = 0; i < 20; i++) {
        fill_sine(buf, FRAME_SIZE, 440.0f, 0.001f);
        audio_pipeline_process(p, buf, FRAME_SIZE);
    }
    float out_rms_val = rms(buf, FRAME_SIZE);

    /* Output should be louder than input */
    TEST_ASSERT(out_rms_val > in_rms_val, "AGC boosted quiet signal");

    float gain_db = agc_get_current_gain_db(s);
    TEST_ASSERT(gain_db > 0.0f, "gain is positive for quiet input");

    audio_pipeline_destroy(p);
    agc_destroy(s);
    TEST_PASS("AGC boosts quiet signal");
    return 0;
}

static int test_agc_limits_loud(void) {
    printf("\n=== test_agc_limits_loud ===\n");

    agc_config_t cfg = {
        .target_dbfs  = -18.0f,
        .max_gain_db  = +6.0f,
        .min_gain_db  = -40.0f,
        .attack_ms    = 5.0f,
        .release_ms   = 20.0f,
        .sample_rate  = SAMPLE_RATE,
    };
    agc_state_t *s = agc_create(&cfg);
    TEST_ASSERT(s != NULL, "AGC created");

    audio_pipeline_t *p = audio_pipeline_create(SAMPLE_RATE, CHANNELS);
    TEST_ASSERT(p != NULL, "pipeline created");

    audio_filter_node_t agc_node = agc_make_node(s);
    audio_pipeline_add_node(p, &agc_node);

    /* Very loud signal (near full scale) — should be attenuated */
    float buf[FRAME_SIZE];
    for (int i = 0; i < 30; i++) {
        fill_sine(buf, FRAME_SIZE, 440.0f, 0.9f);
        audio_pipeline_process(p, buf, FRAME_SIZE);
    }

    /* All output samples should be within [-1, 1] (clipped in AGC) */
    for (int i = 0; i < FRAME_SIZE; i++) {
        TEST_ASSERT(buf[i] <= 1.0f && buf[i] >= -1.0f,
                    "output within clipping range");
    }

    audio_pipeline_destroy(p);
    agc_destroy(s);
    TEST_PASS("AGC limits loud signal to [-1, 1]");
    return 0;
}

/* ── AEC tests ───────────────────────────────────────────────────── */

static int test_aec_create_destroy(void) {
    printf("\n=== test_aec_create_destroy ===\n");

    aec_config_t cfg = {
        .sample_rate      = SAMPLE_RATE,
        .channels         = 1,
        .filter_length_ms = 50,
        .step_size        = 0.5f,
    };
    aec_state_t *s = aec_create(&cfg);
    TEST_ASSERT(s != NULL, "AEC created");

    aec_destroy(s);
    aec_destroy(NULL); /* must not crash */
    TEST_PASS("AEC create/destroy");
    return 0;
}

static int test_aec_pure_echo(void) {
    printf("\n=== test_aec_pure_echo ===\n");

    aec_config_t cfg = {
        .sample_rate      = SAMPLE_RATE,
        .channels         = 1,
        .filter_length_ms = 20,
        .step_size        = 0.8f,
    };
    aec_state_t *s = aec_create(&cfg);
    TEST_ASSERT(s != NULL, "AEC created");

    float ref[FRAME_SIZE], mic[FRAME_SIZE], out[FRAME_SIZE];

    /* Mic == ref (pure echo, no near-end speech) */
    fill_sine(ref, FRAME_SIZE, 440.0f, 0.5f);
    fill_sine(mic, FRAME_SIZE, 440.0f, 0.5f);

    /* Adapt over several frames */
    for (int i = 0; i < 200; i++) {
        fill_sine(ref, FRAME_SIZE, 440.0f, 0.5f);
        fill_sine(mic, FRAME_SIZE, 440.0f, 0.5f);
        aec_process(s, mic, ref, out, FRAME_SIZE);
    }

    float out_rms_val = rms(out, FRAME_SIZE);
    /* After convergence the echo should be largely cancelled */
    TEST_ASSERT(out_rms_val < 0.3f,
                "AEC reduces pure echo after adaptation");

    aec_destroy(s);
    TEST_PASS("AEC converges on pure echo");
    return 0;
}

static int test_aec_set_reference_pipeline(void) {
    printf("\n=== test_aec_set_reference_pipeline ===\n");

    aec_config_t cfg = {
        .sample_rate      = SAMPLE_RATE,
        .channels         = 1,
        .filter_length_ms = 20,
        .step_size        = 0.5f,
    };
    aec_state_t *s = aec_create(&cfg);
    TEST_ASSERT(s != NULL, "AEC created");

    audio_pipeline_t *p = audio_pipeline_create(SAMPLE_RATE, 1);
    TEST_ASSERT(p != NULL, "pipeline created");

    audio_filter_node_t node = aec_make_node(s);
    audio_pipeline_add_node(p, &node);

    float ref[FRAME_SIZE], mic[FRAME_SIZE];
    fill_sine(ref, FRAME_SIZE, 440.0f, 0.5f);
    fill_sine(mic, FRAME_SIZE, 440.0f, 0.5f);

    /* Set reference then process */
    aec_set_reference(s, ref, FRAME_SIZE);
    audio_pipeline_process(p, mic, FRAME_SIZE);
    /* Must not crash; output may vary */

    /* Without reference set, process should be a no-op */
    fill_sine(mic, FRAME_SIZE, 440.0f, 0.5f);
    float before_rms = rms(mic, FRAME_SIZE);
    /* ref_buf is NULL after previous call */
    audio_pipeline_process(p, mic, FRAME_SIZE);
    float after_rms = rms(mic, FRAME_SIZE);
    TEST_ASSERT(fabsf(after_rms - before_rms) < 1e-5f,
                "no-reference pass-through preserves signal");

    audio_pipeline_destroy(p);
    aec_destroy(s);
    TEST_PASS("AEC pipeline set_reference flow");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_pipeline_create_destroy();
    failures += test_pipeline_add_remove();
    failures += test_pipeline_process_passthrough();
    failures += test_pipeline_null_guards();

    failures += test_noise_gate_silences_quiet();
    failures += test_noise_gate_passes_loud();
    failures += test_noise_gate_null();

    failures += test_agc_boosts_quiet();
    failures += test_agc_limits_loud();

    failures += test_aec_create_destroy();
    failures += test_aec_pure_echo();
    failures += test_aec_set_reference_pipeline();

    printf("\n");
    if (failures == 0) {
        printf("ALL AUDIO DSP TESTS PASSED\n");
    } else {
        printf("%d AUDIO DSP TEST(S) FAILED\n", failures);
    }
    return failures ? 1 : 0;
}
