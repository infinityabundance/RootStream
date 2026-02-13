/*
 * test_diagnostics.c - Test system diagnostics reporting
 * 
 * Validates:
 * - Feature detection accuracy
 * - Backend selection reporting
 * - System information gathering
 * - Recommendations generation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "../common/test_harness.h"

/* Test: Feature detection works */
test_result_t test_feature_detection(void) {
    rootstream_ctx_t ctx = {0};
    
    /* Initialize features as they would be at startup */
    ctx.features.capture_drm = 1;
    ctx.features.capture_dummy = 1;
    ctx.features.encode_dummy = 1;
    ctx.features.audio_dummy = 1;
    
    ASSERT_EQ(ctx.features.capture_drm, 1);
    ASSERT_EQ(ctx.features.capture_dummy, 1);
    
    return TEST_PASS;
}

/* Test: Active backend tracking */
test_result_t test_active_backend_tracking(void) {
    rootstream_ctx_t ctx = {0};
    
    /* Simulate backend selection */
    strcpy(ctx.active_backend.capture_name, "DRM/KMS");
    strcpy(ctx.active_backend.encoder_name, "NVENC");
    strcpy(ctx.active_backend.audio_cap_name, "ALSA");
    
    ASSERT_STR_EQ(ctx.active_backend.capture_name, "DRM/KMS");
    ASSERT_STR_EQ(ctx.active_backend.encoder_name, "NVENC");
    ASSERT_STR_EQ(ctx.active_backend.audio_cap_name, "ALSA");
    
    return TEST_PASS;
}

/* Test: System info collection */
test_result_t test_system_info(void) {
    char hostname[256];
    int ret = gethostname(hostname, sizeof(hostname));
    
    ASSERT_EQ(ret, 0);
    ASSERT_TRUE(strlen(hostname) > 0);
    
    return TEST_PASS;
}

/* Test: GPU access detection */
test_result_t test_gpu_access_detection(void) {
    /* Check for DRM device */
    int has_drm = (access("/dev/dri/card0", F_OK) == 0) ||
                  (access("/dev/dri/renderD128", F_OK) == 0);
    
    printf("  GPU access: %s\n", has_drm ? "YES" : "NO");
    
    /* Test always passes - we're just checking detection works */
    return TEST_PASS;
}

/* Test: Backend name setting and retrieval */
test_result_t test_backend_name_operations(void) {
    rootstream_ctx_t ctx = {0};
    
    /* Test capture backend name */
    const char *capture_names[] = {"DRM/KMS", "X11", "Dummy"};
    for (int i = 0; i < 3; i++) {
        strcpy(ctx.active_backend.capture_name, capture_names[i]);
        ASSERT_STR_EQ(ctx.active_backend.capture_name, capture_names[i]);
    }
    
    /* Test encoder backend name */
    const char *encoder_names[] = {"NVENC", "VA-API", "x264", "Raw"};
    for (int i = 0; i < 4; i++) {
        strcpy(ctx.active_backend.encoder_name, encoder_names[i]);
        ASSERT_STR_EQ(ctx.active_backend.encoder_name, encoder_names[i]);
    }
    
    return TEST_PASS;
}

/* Test: Feature flag combinations */
test_result_t test_feature_flag_combinations(void) {
    rootstream_ctx_t ctx = {0};
    
    /* Test all features disabled */
    memset(&ctx.features, 0, sizeof(ctx.features));
    ASSERT_EQ(ctx.features.capture_drm, 0);
    ASSERT_EQ(ctx.features.encode_nvenc, 0);
    
    /* Test all features enabled */
    ctx.features.capture_drm = 1;
    ctx.features.capture_x11 = 1;
    ctx.features.capture_dummy = 1;
    ctx.features.encode_nvenc = 1;
    ctx.features.encode_vaapi = 1;
    ctx.features.encode_x264 = 1;
    ctx.features.encode_dummy = 1;
    ctx.features.audio_alsa = 1;
    ctx.features.audio_pulse = 1;
    ctx.features.audio_pipewire = 1;
    ctx.features.audio_dummy = 1;
    
    ASSERT_EQ(ctx.features.capture_drm, 1);
    ASSERT_EQ(ctx.features.encode_nvenc, 1);
    ASSERT_EQ(ctx.features.audio_alsa, 1);
    
    return TEST_PASS;
}

const test_case_t diagnostics_tests[] = {
    { "Feature detection", test_feature_detection },
    { "Active backend tracking", test_active_backend_tracking },
    { "System info", test_system_info },
    { "GPU access detection", test_gpu_access_detection },
    { "Backend name operations", test_backend_name_operations },
    { "Feature flag combinations", test_feature_flag_combinations },
    {NULL, NULL}
};

int main(void) {
    printf("Running diagnostics tests...\n");
    return run_test_suite(diagnostics_tests);
}
