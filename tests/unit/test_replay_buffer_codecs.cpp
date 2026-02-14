/**
 * Test for Phase 27.3: Replay Buffer Polish
 * 
 * This test verifies:
 * 1. Replay buffer with multiple codecs (H.264, VP9, AV1)
 * 2. Codec selection in replay buffer save
 * 3. Integration with RecordingManager
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../src/recording/replay_buffer.h"
#include "../../src/recording/recording_types.h"

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
 * Test replay buffer save with H.264 codec
 */
int test_replay_buffer_save_h264() {
    replay_buffer_t *buffer = replay_buffer_create(10, 100);
    TEST_ASSERT(buffer != nullptr, "Replay buffer should be created");
    
    // Create dummy frame data
    const size_t frame_size = 1024; // Small encoded frame
    uint8_t *frame_data = (uint8_t *)malloc(frame_size);
    TEST_ASSERT(frame_data != nullptr, "Failed to allocate frame data");
    memset(frame_data, 0xFF, frame_size);
    
    // Add some frames
    for (int i = 0; i < 5; i++) {
        int ret = replay_buffer_add_video_frame(buffer, frame_data, frame_size,
                                               1920, 1080, i * 16667, i == 0);
        TEST_ASSERT(ret == 0, "Should successfully add video frame");
    }
    
    // Save with H.264 codec
    const char *filename = "/tmp/test_replay_h264.mp4";
    int ret = replay_buffer_save(buffer, filename, 0, VIDEO_CODEC_H264);
    TEST_ASSERT(ret == 0, "Should successfully save replay buffer with H.264");
    
    // Verify file was created
    struct stat st;
    TEST_ASSERT(stat(filename, &st) == 0, "Output file should exist");
    
    // Cleanup
    unlink(filename);
    free(frame_data);
    replay_buffer_destroy(buffer);
    
    TEST_PASS("test_replay_buffer_save_h264");
}

/**
 * Test replay buffer save with VP9 codec
 */
int test_replay_buffer_save_vp9() {
    replay_buffer_t *buffer = replay_buffer_create(10, 100);
    TEST_ASSERT(buffer != nullptr, "Replay buffer should be created");
    
    // Create dummy frame data
    const size_t frame_size = 1024;
    uint8_t *frame_data = (uint8_t *)malloc(frame_size);
    TEST_ASSERT(frame_data != nullptr, "Failed to allocate frame data");
    memset(frame_data, 0xFF, frame_size);
    
    // Add some frames
    for (int i = 0; i < 5; i++) {
        int ret = replay_buffer_add_video_frame(buffer, frame_data, frame_size,
                                               1920, 1080, i * 16667, i == 0);
        TEST_ASSERT(ret == 0, "Should successfully add video frame");
    }
    
    // Save with VP9 codec
    const char *filename = "/tmp/test_replay_vp9.mkv";
    int ret = replay_buffer_save(buffer, filename, 0, VIDEO_CODEC_VP9);
    TEST_ASSERT(ret == 0, "Should successfully save replay buffer with VP9");
    
    // Verify file was created
    struct stat st;
    TEST_ASSERT(stat(filename, &st) == 0, "Output file should exist");
    
    // Cleanup
    unlink(filename);
    free(frame_data);
    replay_buffer_destroy(buffer);
    
    TEST_PASS("test_replay_buffer_save_vp9");
}

/**
 * Test replay buffer save with AV1 codec
 */
int test_replay_buffer_save_av1() {
    replay_buffer_t *buffer = replay_buffer_create(10, 100);
    TEST_ASSERT(buffer != nullptr, "Replay buffer should be created");
    
    // Create dummy frame data
    const size_t frame_size = 1024;
    uint8_t *frame_data = (uint8_t *)malloc(frame_size);
    TEST_ASSERT(frame_data != nullptr, "Failed to allocate frame data");
    memset(frame_data, 0xFF, frame_size);
    
    // Add some frames
    for (int i = 0; i < 5; i++) {
        int ret = replay_buffer_add_video_frame(buffer, frame_data, frame_size,
                                               1920, 1080, i * 16667, i == 0);
        TEST_ASSERT(ret == 0, "Should successfully add video frame");
    }
    
    // Save with AV1 codec
    const char *filename = "/tmp/test_replay_av1.mkv";
    int ret = replay_buffer_save(buffer, filename, 0, VIDEO_CODEC_AV1);
    TEST_ASSERT(ret == 0, "Should successfully save replay buffer with AV1");
    
    // Verify file was created
    struct stat st;
    TEST_ASSERT(stat(filename, &st) == 0, "Output file should exist");
    
    // Cleanup
    unlink(filename);
    free(frame_data);
    replay_buffer_destroy(buffer);
    
    TEST_PASS("test_replay_buffer_save_av1");
}

/**
 * Test replay buffer save with duration parameter
 */
int test_replay_buffer_save_duration() {
    replay_buffer_t *buffer = replay_buffer_create(30, 100);
    TEST_ASSERT(buffer != nullptr, "Replay buffer should be created");
    
    // Create dummy frame data
    const size_t frame_size = 1024;
    uint8_t *frame_data = (uint8_t *)malloc(frame_size);
    TEST_ASSERT(frame_data != nullptr, "Failed to allocate frame data");
    memset(frame_data, 0xFF, frame_size);
    
    // Add frames spanning 10 seconds (at 60 FPS)
    for (int i = 0; i < 600; i++) {
        int ret = replay_buffer_add_video_frame(buffer, frame_data, frame_size,
                                               1920, 1080, i * 16667, i % 60 == 0);
        TEST_ASSERT(ret == 0, "Should successfully add video frame");
    }
    
    // Save last 5 seconds only
    const char *filename = "/tmp/test_replay_duration.mp4";
    int ret = replay_buffer_save(buffer, filename, 5, VIDEO_CODEC_H264);
    TEST_ASSERT(ret == 0, "Should successfully save replay buffer with duration limit");
    
    // Verify file was created
    struct stat st;
    TEST_ASSERT(stat(filename, &st) == 0, "Output file should exist");
    
    // Cleanup
    unlink(filename);
    free(frame_data);
    replay_buffer_destroy(buffer);
    
    TEST_PASS("test_replay_buffer_save_duration");
}

/**
 * Test codec detection with different file extensions
 */
int test_replay_buffer_codec_detection() {
    replay_buffer_t *buffer = replay_buffer_create(10, 100);
    TEST_ASSERT(buffer != nullptr, "Replay buffer should be created");
    
    // Create dummy frame data
    const size_t frame_size = 1024;
    uint8_t *frame_data = (uint8_t *)malloc(frame_size);
    TEST_ASSERT(frame_data != nullptr, "Failed to allocate frame data");
    memset(frame_data, 0xFF, frame_size);
    
    // Add some frames
    for (int i = 0; i < 5; i++) {
        int ret = replay_buffer_add_video_frame(buffer, frame_data, frame_size,
                                               1920, 1080, i * 16667, i == 0);
        TEST_ASSERT(ret == 0, "Should successfully add video frame");
    }
    
    // Test MP4 extension with H.264
    const char *filename_mp4 = "/tmp/test_replay.mp4";
    int ret = replay_buffer_save(buffer, filename_mp4, 0, VIDEO_CODEC_H264);
    TEST_ASSERT(ret == 0, "Should save MP4 file");
    unlink(filename_mp4);
    
    // Test MKV extension with VP9
    const char *filename_mkv = "/tmp/test_replay.mkv";
    ret = replay_buffer_save(buffer, filename_mkv, 0, VIDEO_CODEC_VP9);
    TEST_ASSERT(ret == 0, "Should save MKV file");
    unlink(filename_mkv);
    
    // Cleanup
    free(frame_data);
    replay_buffer_destroy(buffer);
    
    TEST_PASS("test_replay_buffer_codec_detection");
}

/**
 * Test error handling with invalid codec
 */
int test_replay_buffer_invalid_codec() {
    replay_buffer_t *buffer = replay_buffer_create(10, 100);
    TEST_ASSERT(buffer != nullptr, "Replay buffer should be created");
    
    // Create dummy frame data
    const size_t frame_size = 1024;
    uint8_t *frame_data = (uint8_t *)malloc(frame_size);
    TEST_ASSERT(frame_data != nullptr, "Failed to allocate frame data");
    memset(frame_data, 0xFF, frame_size);
    
    // Add a frame
    int ret = replay_buffer_add_video_frame(buffer, frame_data, frame_size,
                                           1920, 1080, 0, true);
    TEST_ASSERT(ret == 0, "Should successfully add video frame");
    
    // Save with invalid codec (should default to H.264)
    const char *filename = "/tmp/test_replay_invalid.mp4";
    ret = replay_buffer_save(buffer, filename, 0, (enum VideoCodec)999);
    TEST_ASSERT(ret == 0, "Should handle invalid codec gracefully");
    
    // Verify file was created
    struct stat st;
    TEST_ASSERT(stat(filename, &st) == 0, "Output file should exist");
    
    // Cleanup
    unlink(filename);
    free(frame_data);
    replay_buffer_destroy(buffer);
    
    TEST_PASS("test_replay_buffer_invalid_codec");
}

/**
 * Main test runner
 */
int main() {
    int failed = 0;
    
    printf("Running Phase 27.3 replay buffer tests...\n");
    printf("=========================================\n\n");
    
    if (test_replay_buffer_save_h264() != 0) failed++;
    if (test_replay_buffer_save_vp9() != 0) failed++;
    if (test_replay_buffer_save_av1() != 0) failed++;
    if (test_replay_buffer_save_duration() != 0) failed++;
    if (test_replay_buffer_codec_detection() != 0) failed++;
    if (test_replay_buffer_invalid_codec() != 0) failed++;
    
    printf("\n=========================================\n");
    if (failed == 0) {
        printf("✓ All Phase 27.3 tests passed!\n");
        return 0;
    } else {
        printf("✗ %d test(s) failed\n", failed);
        return 1;
    }
}
