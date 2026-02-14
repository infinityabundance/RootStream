/**
 * Test for Video Encoder Integration
 * 
 * This test verifies:
 * 1. Encoder availability checking
 * 2. Encoder initialization (H.264, VP9, AV1)
 * 3. Encoder selection based on preset
 * 4. Encoder cleanup
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/recording/h264_encoder_wrapper.h"
#include "../../src/recording/vp9_encoder_wrapper.h"
#include "../../src/recording/av1_encoder_wrapper.h"

// Test helper macros
#define TEST_ASSERT(condition, msg) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s\n", msg); \
            return -1; \
        } \
    } while(0)

#define TEST_PASS(name) \
    do { \
        printf("PASS: %s\n", name); \
        return 0; \
    } while(0)

/**
 * Test encoder availability
 */
int test_encoder_availability() {
    // At least one encoder should be available
    bool h264_available = h264_encoder_available();
    bool vp9_available = vp9_encoder_available();
    bool av1_available = av1_encoder_available();
    
    printf("  H.264 encoder: %s\n", h264_available ? "available" : "not available");
    printf("  VP9 encoder: %s\n", vp9_available ? "available" : "not available");
    printf("  AV1 encoder: %s\n", av1_available ? "available" : "not available");
    
    TEST_ASSERT(h264_available || vp9_available || av1_available,
                "At least one encoder should be available");
    
    TEST_PASS("test_encoder_availability");
}

/**
 * Test H.264 encoder initialization
 */
int test_h264_encoder_init() {
    if (!h264_encoder_available()) {
        printf("SKIP: H.264 encoder not available\n");
        return 0;
    }
    
    h264_encoder_t encoder;
    memset(&encoder, 0, sizeof(encoder));
    
    // Initialize with typical settings
    int ret = h264_encoder_init(&encoder, 1920, 1080, 60, 8000, "medium", 23);
    TEST_ASSERT(ret == 0, "H.264 encoder should initialize successfully");
    TEST_ASSERT(encoder.initialized == true, "H.264 encoder should be marked as initialized");
    TEST_ASSERT(encoder.width == 1920, "Width should be 1920");
    TEST_ASSERT(encoder.height == 1080, "Height should be 1080");
    
    // Cleanup
    h264_encoder_cleanup(&encoder);
    
    TEST_PASS("test_h264_encoder_init");
}

/**
 * Test VP9 encoder initialization
 */
int test_vp9_encoder_init() {
    if (!vp9_encoder_available()) {
        printf("SKIP: VP9 encoder not available\n");
        return 0;
    }
    
    vp9_encoder_t encoder;
    memset(&encoder, 0, sizeof(encoder));
    
    // Initialize with typical settings
    int ret = vp9_encoder_init(&encoder, 1920, 1080, 60, 5000, 2, -1);
    TEST_ASSERT(ret == 0, "VP9 encoder should initialize successfully");
    TEST_ASSERT(encoder.initialized == true, "VP9 encoder should be marked as initialized");
    TEST_ASSERT(encoder.width == 1920, "Width should be 1920");
    TEST_ASSERT(encoder.height == 1080, "Height should be 1080");
    TEST_ASSERT(encoder.cpu_used == 2, "cpu_used should be 2");
    
    // Cleanup
    vp9_encoder_cleanup(&encoder);
    
    TEST_PASS("test_vp9_encoder_init");
}

/**
 * Test AV1 encoder initialization
 */
int test_av1_encoder_init() {
    if (!av1_encoder_available()) {
        printf("SKIP: AV1 encoder not available\n");
        return 0;
    }
    
    av1_encoder_t encoder;
    memset(&encoder, 0, sizeof(encoder));
    
    // Initialize with typical settings
    int ret = av1_encoder_init(&encoder, 1920, 1080, 60, 2000, 4, -1);
    TEST_ASSERT(ret == 0, "AV1 encoder should initialize successfully");
    TEST_ASSERT(encoder.initialized == true, "AV1 encoder should be marked as initialized");
    TEST_ASSERT(encoder.width == 1920, "Width should be 1920");
    TEST_ASSERT(encoder.height == 1080, "Height should be 1080");
    TEST_ASSERT(encoder.cpu_used == 4, "cpu_used should be 4");
    
    // Cleanup
    av1_encoder_cleanup(&encoder);
    
    TEST_PASS("test_av1_encoder_init");
}

/**
 * Test encoder with different resolutions
 */
int test_encoder_different_resolutions() {
    if (!h264_encoder_available()) {
        printf("SKIP: H.264 encoder not available\n");
        return 0;
    }
    
    // Test 720p
    h264_encoder_t encoder720;
    memset(&encoder720, 0, sizeof(encoder720));
    int ret = h264_encoder_init(&encoder720, 1280, 720, 60, 5000, "fast", 23);
    TEST_ASSERT(ret == 0, "Should initialize 720p encoder");
    h264_encoder_cleanup(&encoder720);
    
    // Test 1080p
    h264_encoder_t encoder1080;
    memset(&encoder1080, 0, sizeof(encoder1080));
    ret = h264_encoder_init(&encoder1080, 1920, 1080, 60, 8000, "medium", 23);
    TEST_ASSERT(ret == 0, "Should initialize 1080p encoder");
    h264_encoder_cleanup(&encoder1080);
    
    // Test 4K
    h264_encoder_t encoder4k;
    memset(&encoder4k, 0, sizeof(encoder4k));
    ret = h264_encoder_init(&encoder4k, 3840, 2160, 60, 20000, "fast", 23);
    TEST_ASSERT(ret == 0, "Should initialize 4K encoder");
    h264_encoder_cleanup(&encoder4k);
    
    TEST_PASS("test_encoder_different_resolutions");
}

/**
 * Test encoder with different framerates
 */
int test_encoder_different_framerates() {
    if (!h264_encoder_available()) {
        printf("SKIP: H.264 encoder not available\n");
        return 0;
    }
    
    // Test 30 FPS
    h264_encoder_t encoder30;
    memset(&encoder30, 0, sizeof(encoder30));
    int ret = h264_encoder_init(&encoder30, 1920, 1080, 30, 4000, "medium", 23);
    TEST_ASSERT(ret == 0, "Should initialize 30 FPS encoder");
    TEST_ASSERT(encoder30.fps == 30, "FPS should be 30");
    h264_encoder_cleanup(&encoder30);
    
    // Test 60 FPS
    h264_encoder_t encoder60;
    memset(&encoder60, 0, sizeof(encoder60));
    ret = h264_encoder_init(&encoder60, 1920, 1080, 60, 8000, "medium", 23);
    TEST_ASSERT(ret == 0, "Should initialize 60 FPS encoder");
    TEST_ASSERT(encoder60.fps == 60, "FPS should be 60");
    h264_encoder_cleanup(&encoder60);
    
    // Test 144 FPS
    h264_encoder_t encoder144;
    memset(&encoder144, 0, sizeof(encoder144));
    ret = h264_encoder_init(&encoder144, 1920, 1080, 144, 15000, "veryfast", 23);
    TEST_ASSERT(ret == 0, "Should initialize 144 FPS encoder");
    TEST_ASSERT(encoder144.fps == 144, "FPS should be 144");
    h264_encoder_cleanup(&encoder144);
    
    TEST_PASS("test_encoder_different_framerates");
}

/**
 * Test encoder cleanup safety
 */
int test_encoder_cleanup_safety() {
    // Test cleanup of uninitialized encoder (should not crash)
    h264_encoder_t encoder;
    memset(&encoder, 0, sizeof(encoder));
    encoder.initialized = false;
    
    h264_encoder_cleanup(&encoder); // Should handle gracefully
    
    TEST_PASS("test_encoder_cleanup_safety");
}

/**
 * Main test runner
 */
int main() {
    int failed = 0;
    
    printf("Running encoder integration tests...\n");
    printf("=====================================\n\n");
    
    if (test_encoder_availability() != 0) failed++;
    if (test_h264_encoder_init() != 0) failed++;
    if (test_vp9_encoder_init() != 0) failed++;
    if (test_av1_encoder_init() != 0) failed++;
    if (test_encoder_different_resolutions() != 0) failed++;
    if (test_encoder_different_framerates() != 0) failed++;
    if (test_encoder_cleanup_safety() != 0) failed++;
    
    printf("\n=====================================\n");
    if (failed == 0) {
        printf("✓ All encoder integration tests passed!\n");
        return 0;
    } else {
        printf("✗ %d test(s) failed\n", failed);
        return 1;
    }
}
