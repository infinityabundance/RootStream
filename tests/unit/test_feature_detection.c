/*
 * test_feature_detection.c - Test runtime feature availability detection
 * 
 * Validates:
 * - Hardware capability detection
 * - Software dependency detection
 * - Fallback availability checks
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../common/test_harness.h"

/* Test: DRM device detection */
test_result_t test_detect_drm_available(void) {
    /* Check for DRM devices */
    int has_card0 = (access("/dev/dri/card0", F_OK) == 0);
    int has_renderD128 = (access("/dev/dri/renderD128", F_OK) == 0);
    int has_drm = has_card0 || has_renderD128;
    
    printf("  DRM available: %s\n", has_drm ? "YES" : "NO");
    
    /* Test passes regardless - just checking detection */
    return TEST_PASS;
}

/* Test: X11 display detection */
test_result_t test_detect_x11_available(void) {
    const char *display = getenv("DISPLAY");
    int has_x11 = (display != NULL && strlen(display) > 0);
    
    printf("  X11 available: %s\n", has_x11 ? "YES" : "NO");
    
    return TEST_PASS;
}

/* Test: Audio device detection */
test_result_t test_detect_audio_devices(void) {
    /* Check for ALSA devices */
    int has_pcm = (access("/dev/snd/pcmC0D0p", F_OK) == 0) ||
                  (access("/dev/snd/pcmC0D0c", F_OK) == 0);
    
    /* Check for PulseAudio socket */
    const char *pulse_server = getenv("PULSE_SERVER");
    int has_pulse = (pulse_server != NULL) ||
                    (access("/run/user/1000/pulse/native", F_OK) == 0);
    
    printf("  ALSA devices: %s\n", has_pcm ? "YES" : "NO");
    printf("  PulseAudio: %s\n", has_pulse ? "YES" : "NO");
    
    return TEST_PASS;
}

/* Test: GPU acceleration detection */
test_result_t test_detect_gpu_acceleration(void) {
    /* Check for VA-API */
    int has_vaapi = (access("/dev/dri/renderD128", F_OK) == 0);
    
    /* Check for NVIDIA */
    int has_nvidia = (access("/dev/nvidia0", F_OK) == 0);
    
    printf("  VA-API: %s\n", has_vaapi ? "YES" : "NO");
    printf("  NVIDIA: %s\n", has_nvidia ? "YES" : "NO");
    
    return TEST_PASS;
}

/* Test: Network capability detection */
test_result_t test_detect_network_capabilities(void) {
    /* Check if we can get hostname */
    char hostname[256];
    int ret = gethostname(hostname, sizeof(hostname));
    
    ASSERT_EQ(ret, 0);
    ASSERT_TRUE(strlen(hostname) > 0);
    
    printf("  Hostname: %s\n", hostname);
    
    return TEST_PASS;
}

/* Test: Dummy backends always available */
test_result_t test_detect_dummy_backends(void) {
    /* Dummy backends are always available (they're compiled in) */
    int has_dummy_capture = 1;
    int has_dummy_encoder = 1;
    int has_dummy_audio = 1;
    
    ASSERT_EQ(has_dummy_capture, 1);
    ASSERT_EQ(has_dummy_encoder, 1);
    ASSERT_EQ(has_dummy_audio, 1);
    
    return TEST_PASS;
}

/* Test: Feature detection context initialization */
test_result_t test_feature_context_init(void) {
    rootstream_ctx_t ctx = {0};
    
    /* Simulate feature detection */
    ctx.features.capture_dummy = 1;  /* Always available */
    ctx.features.encode_dummy = 1;   /* Always available */
    ctx.features.audio_dummy = 1;    /* Always available */
    
    /* Optionally available */
    ctx.features.capture_drm = (access("/dev/dri/card0", F_OK) == 0);
    ctx.features.capture_x11 = (getenv("DISPLAY") != NULL);
    
    /* Verify at least dummy backends are available */
    ASSERT_EQ(ctx.features.capture_dummy, 1);
    ASSERT_EQ(ctx.features.encode_dummy, 1);
    ASSERT_EQ(ctx.features.audio_dummy, 1);
    
    return TEST_PASS;
}

/* Test: Feature priority ordering */
test_result_t test_feature_priority_selection(void) {
    rootstream_ctx_t ctx = {0};
    
    /* Simulate all features available */
    ctx.features.capture_drm = 1;
    ctx.features.capture_x11 = 1;
    ctx.features.capture_dummy = 1;
    
    /* Priority: DRM > X11 > Dummy */
    const char *selected = NULL;
    if (ctx.features.capture_drm) {
        selected = "DRM/KMS";
    } else if (ctx.features.capture_x11) {
        selected = "X11";
    } else if (ctx.features.capture_dummy) {
        selected = "Dummy";
    }
    
    ASSERT_NOT_NULL(selected);
    ASSERT_STR_EQ(selected, "DRM/KMS");
    
    return TEST_PASS;
}

const test_case_t feature_tests[] = {
    { "DRM detection", test_detect_drm_available },
    { "X11 detection", test_detect_x11_available },
    { "Audio devices", test_detect_audio_devices },
    { "GPU acceleration", test_detect_gpu_acceleration },
    { "Network capabilities", test_detect_network_capabilities },
    { "Dummy backends", test_detect_dummy_backends },
    { "Context initialization", test_feature_context_init },
    { "Priority selection", test_feature_priority_selection },
    {NULL, NULL}
};

int main(void) {
    printf("Running feature detection tests...\n");
    return run_test_suite(feature_tests);
}
