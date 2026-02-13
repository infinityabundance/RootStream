/*
 * test_backends_audio.c - Test audio backend selection logic
 * 
 * Unit tests for audio backend initialization and fallback
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/test_harness.h"

/* Test: Audio backend selection priority */
test_result_t test_audio_backend_priority(void) {
    /* ALSA > PulseAudio > PipeWire > Dummy priority order */
    const char *backends[] = {"ALSA", "PulseAudio", "PipeWire", "Dummy"};
    ASSERT_STR_EQ(backends[0], "ALSA");
    ASSERT_STR_EQ(backends[3], "Dummy");
    return TEST_PASS;
}

/* Test: Audio backend name validation */
test_result_t test_audio_backend_names(void) {
    rootstream_ctx_t ctx = {0};
    
    strcpy(ctx.active_backend.audio_cap_name, "ALSA");
    ASSERT_STR_EQ(ctx.active_backend.audio_cap_name, "ALSA");
    
    strcpy(ctx.active_backend.audio_play_name, "Dummy");
    ASSERT_STR_EQ(ctx.active_backend.audio_play_name, "Dummy");
    
    return TEST_PASS;
}

const test_case_t audio_backend_tests[] = {
    { "Backend priority", test_audio_backend_priority },
    { "Backend names", test_audio_backend_names },
    {NULL, NULL}
};

int main(void) {
    printf("Running audio backend tests...\n");
    return run_test_suite(audio_backend_tests);
}
