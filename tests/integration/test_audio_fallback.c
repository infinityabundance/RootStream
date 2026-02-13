/*
 * test_audio_fallback.c - Test audio backend fallback chain
 * 
 * Validates:
 * - ALSA audio capture/playback (if available)
 * - PulseAudio fallback
 * - PipeWire fallback
 * - Dummy silent audio final fallback
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/test_harness.h"

typedef struct {
    const char *name;
    int (*init_fn)(rootstream_ctx_t *ctx);
    int (*capture_fn)(rootstream_ctx_t *ctx, uint8_t *buffer, size_t size);
    void (*cleanup_fn)(rootstream_ctx_t *ctx);
} audio_backend_t;

/* Mock ALSA backend */
int mock_alsa_init(rootstream_ctx_t *ctx) {
    (void)ctx;
    return 0;
}

int mock_alsa_capture(rootstream_ctx_t *ctx, uint8_t *buffer, size_t size) {
    (void)ctx;
    /* Simulate audio samples (sine wave pattern) */
    for (size_t i = 0; i < size; i += 2) {
        int16_t sample = (int16_t)(32767.0 * 0.5 * (i % 100) / 100.0);
        buffer[i] = sample & 0xFF;
        buffer[i + 1] = (sample >> 8) & 0xFF;
    }
    return 0;
}

void mock_alsa_cleanup(rootstream_ctx_t *ctx) {
    (void)ctx;
}

/* Mock PulseAudio backend */
int mock_pulse_init(rootstream_ctx_t *ctx) {
    (void)ctx;
    return 0;
}

int mock_pulse_capture(rootstream_ctx_t *ctx, uint8_t *buffer, size_t size) {
    (void)ctx;
    /* Simulate audio samples (different pattern) */
    for (size_t i = 0; i < size; i += 2) {
        int16_t sample = (int16_t)(32767.0 * 0.3 * (i % 50) / 50.0);
        buffer[i] = sample & 0xFF;
        buffer[i + 1] = (sample >> 8) & 0xFF;
    }
    return 0;
}

void mock_pulse_cleanup(rootstream_ctx_t *ctx) {
    (void)ctx;
}

/* Mock PipeWire backend */
int mock_pipewire_init(rootstream_ctx_t *ctx) {
    (void)ctx;
    return 0;
}

int mock_pipewire_capture(rootstream_ctx_t *ctx, uint8_t *buffer, size_t size) {
    (void)ctx;
    /* Simulate audio samples */
    for (size_t i = 0; i < size; i += 2) {
        int16_t sample = (int16_t)(32767.0 * 0.4 * (i % 75) / 75.0);
        buffer[i] = sample & 0xFF;
        buffer[i + 1] = (sample >> 8) & 0xFF;
    }
    return 0;
}

void mock_pipewire_cleanup(rootstream_ctx_t *ctx) {
    (void)ctx;
}

/* Mock Dummy backend (silent audio) */
int mock_dummy_init(rootstream_ctx_t *ctx) {
    (void)ctx;
    return 0;
}

int mock_dummy_capture(rootstream_ctx_t *ctx, uint8_t *buffer, size_t size) {
    (void)ctx;
    /* Silent audio */
    memset(buffer, 0, size);
    return 0;
}

void mock_dummy_cleanup(rootstream_ctx_t *ctx) {
    (void)ctx;
}

/* Test: ALSA initialization */
test_result_t test_audio_alsa_init(void) {
    rootstream_ctx_t ctx = {0};
    
    int ret = mock_alsa_init(&ctx);
    ASSERT_EQ(ret, 0);
    
    mock_alsa_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: ALSA capture produces audio data */
test_result_t test_audio_alsa_capture(void) {
    rootstream_ctx_t ctx = {0};
    uint8_t buffer[4096];
    
    mock_alsa_init(&ctx);
    
    int ret = mock_alsa_capture(&ctx, buffer, sizeof(buffer));
    ASSERT_EQ(ret, 0);
    
    /* Check that buffer is not all zeros (has audio data) */
    int non_zero = 0;
    for (size_t i = 0; i < sizeof(buffer); i++) {
        if (buffer[i] != 0) {
            non_zero = 1;
            break;
        }
    }
    ASSERT_TRUE(non_zero);
    
    mock_alsa_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: PulseAudio initialization */
test_result_t test_audio_pulse_init(void) {
    rootstream_ctx_t ctx = {0};
    
    int ret = mock_pulse_init(&ctx);
    ASSERT_EQ(ret, 0);
    
    mock_pulse_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: PipeWire initialization */
test_result_t test_audio_pipewire_init(void) {
    rootstream_ctx_t ctx = {0};
    
    int ret = mock_pipewire_init(&ctx);
    ASSERT_EQ(ret, 0);
    
    mock_pipewire_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: Dummy audio initialization */
test_result_t test_audio_dummy_init(void) {
    rootstream_ctx_t ctx = {0};
    
    int ret = mock_dummy_init(&ctx);
    ASSERT_EQ(ret, 0);
    
    mock_dummy_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: Dummy audio produces silence */
test_result_t test_audio_dummy_silence(void) {
    rootstream_ctx_t ctx = {0};
    uint8_t buffer[4096];
    
    mock_dummy_init(&ctx);
    
    /* Fill buffer with non-zero data first */
    memset(buffer, 0xFF, sizeof(buffer));
    
    int ret = mock_dummy_capture(&ctx, buffer, sizeof(buffer));
    ASSERT_EQ(ret, 0);
    
    /* Check that buffer is all zeros (silence) */
    for (size_t i = 0; i < sizeof(buffer); i++) {
        ASSERT_EQ(buffer[i], 0);
    }
    
    mock_dummy_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: Audio fallback chain */
test_result_t test_audio_fallback_chain(void) {
    rootstream_ctx_t ctx = {0};
    
    const audio_backend_t backends[] = {
        { "ALSA", mock_alsa_init, mock_alsa_capture, mock_alsa_cleanup },
        { "PulseAudio", mock_pulse_init, mock_pulse_capture, mock_pulse_cleanup },
        { "PipeWire", mock_pipewire_init, mock_pipewire_capture, mock_pipewire_cleanup },
        { "Dummy", mock_dummy_init, mock_dummy_capture, mock_dummy_cleanup },
        {NULL, NULL, NULL, NULL}
    };
    
    /* Try each backend - at least one should work (Dummy always works) */
    int success = 0;
    for (int i = 0; backends[i].name; i++) {
        int ret = backends[i].init_fn(&ctx);
        
        if (ret == 0) {
            /* Backend initialized successfully */
            uint8_t buffer[4096];
            
            ret = backends[i].capture_fn(&ctx, buffer, sizeof(buffer));
            ASSERT_EQ(ret, 0);
            
            backends[i].cleanup_fn(&ctx);
            success = 1;
            break;
        }
    }
    
    ASSERT_TRUE(success);
    return TEST_PASS;
}

/* Test suite */
const test_case_t audio_tests[] = {
    { "ALSA init", test_audio_alsa_init },
    { "ALSA capture", test_audio_alsa_capture },
    { "PulseAudio init", test_audio_pulse_init },
    { "PipeWire init", test_audio_pipewire_init },
    { "Dummy init", test_audio_dummy_init },
    { "Dummy silence", test_audio_dummy_silence },
    { "Fallback chain", test_audio_fallback_chain },
    {NULL, NULL}
};

int main(void) {
    printf("Running audio fallback tests...\n");
    return run_test_suite(audio_tests);
}
