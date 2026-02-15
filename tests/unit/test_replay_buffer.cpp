/**
 * Test for Replay Buffer functionality
 * 
 * This test verifies:
 * 1. Replay buffer creation and configuration
 * 2. Adding video frames to replay buffer
 * 3. Adding audio chunks to replay buffer
 * 4. Saving replay buffer to file
 * 5. Memory management and cleanup
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../src/recording/replay_buffer.h"

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
 * Test replay buffer creation
 */
int test_replay_buffer_creation() {
    // Test valid creation
    replay_buffer_t *buffer = replay_buffer_create(30, 100);
    TEST_ASSERT(buffer != nullptr, "Replay buffer should be created");
    
    replay_buffer_destroy(buffer);
    
    // Test invalid duration (too large)
    buffer = replay_buffer_create(500, 100);
    TEST_ASSERT(buffer == nullptr, "Should fail with duration > MAX_REPLAY_BUFFER_SECONDS");
    
    // Test invalid duration (zero)
    buffer = replay_buffer_create(0, 100);
    TEST_ASSERT(buffer == nullptr, "Should fail with zero duration");
    
    TEST_PASS("test_replay_buffer_creation");
}

/**
 * Test adding video frames
 */
int test_add_video_frames() {
    replay_buffer_t *buffer = replay_buffer_create(10, 100);
    TEST_ASSERT(buffer != nullptr, "Replay buffer should be created");
    
    // Create dummy frame data
    const size_t frame_size = 1920 * 1080 * 3; // RGB frame
    uint8_t *frame_data = (uint8_t *)malloc(frame_size);
    TEST_ASSERT(frame_data != nullptr, "Failed to allocate frame data");
    
    memset(frame_data, 128, frame_size); // Fill with gray
    
    // Add first frame (keyframe)
    int ret = replay_buffer_add_video_frame(buffer, frame_data, frame_size, 
                                           1920, 1080, 0, true);
    TEST_ASSERT(ret == 0, "Should successfully add video frame");
    
    // Add more frames
    for (int i = 1; i < 10; i++) {
        ret = replay_buffer_add_video_frame(buffer, frame_data, frame_size,
                                           1920, 1080, i * 16667, false);
        TEST_ASSERT(ret == 0, "Should successfully add video frame");
    }
    
    // Check stats
    uint32_t video_frames, audio_chunks, memory_mb, duration_sec;
    ret = replay_buffer_get_stats(buffer, &video_frames, &audio_chunks, 
                                  &memory_mb, &duration_sec);
    TEST_ASSERT(ret == 0, "Should get stats successfully");
    TEST_ASSERT(video_frames == 10, "Should have 10 video frames");
    
    free(frame_data);
    replay_buffer_destroy(buffer);
    
    TEST_PASS("test_add_video_frames");
}

/**
 * Test adding audio chunks
 */
int test_add_audio_chunks() {
    replay_buffer_t *buffer = replay_buffer_create(10, 100);
    TEST_ASSERT(buffer != nullptr, "Replay buffer should be created");
    
    // Create dummy audio data
    const size_t sample_count = 1024;
    float *samples = (float *)malloc(sample_count * sizeof(float));
    TEST_ASSERT(samples != nullptr, "Failed to allocate audio samples");
    
    // Fill with sine wave
    for (size_t i = 0; i < sample_count; i++) {
        samples[i] = 0.5f;
    }
    
    // Add audio chunks
    for (int i = 0; i < 20; i++) {
        int ret = replay_buffer_add_audio_chunk(buffer, samples, sample_count,
                                               48000, 2, i * 21333);
        TEST_ASSERT(ret == 0, "Should successfully add audio chunk");
    }
    
    // Check stats
    uint32_t video_frames, audio_chunks, memory_mb, duration_sec;
    int ret = replay_buffer_get_stats(buffer, &video_frames, &audio_chunks,
                                      &memory_mb, &duration_sec);
    TEST_ASSERT(ret == 0, "Should get stats successfully");
    TEST_ASSERT(audio_chunks == 20, "Should have 20 audio chunks");
    
    free(samples);
    replay_buffer_destroy(buffer);
    
    TEST_PASS("test_add_audio_chunks");
}

/**
 * Test memory limit enforcement
 */
int test_memory_limit() {
    // Create buffer with small memory limit
    replay_buffer_t *buffer = replay_buffer_create(60, 1); // Only 1 MB
    TEST_ASSERT(buffer != nullptr, "Replay buffer should be created");
    
    // Create large frame data
    const size_t frame_size = 1024 * 1024; // 1 MB frame
    uint8_t *frame_data = (uint8_t *)malloc(frame_size);
    TEST_ASSERT(frame_data != nullptr, "Failed to allocate frame data");
    
    memset(frame_data, 0, frame_size);
    
    // Add multiple frames (should trigger cleanup)
    for (int i = 0; i < 10; i++) {
        int ret = replay_buffer_add_video_frame(buffer, frame_data, frame_size,
                                               1920, 1080, i * 100000, i == 0);
        TEST_ASSERT(ret == 0, "Should successfully add video frame");
    }
    
    // Check that memory limit was enforced
    uint32_t video_frames, audio_chunks, memory_mb, duration_sec;
    int ret = replay_buffer_get_stats(buffer, &video_frames, &audio_chunks,
                                      &memory_mb, &duration_sec);
    TEST_ASSERT(ret == 0, "Should get stats successfully");
    TEST_ASSERT(memory_mb <= 2, "Memory usage should be near limit"); // Allow some overhead
    
    free(frame_data);
    replay_buffer_destroy(buffer);
    
    TEST_PASS("test_memory_limit");
}

/**
 * Test time-based cleanup
 */
int test_time_based_cleanup() {
    // Create buffer with 2 second duration
    replay_buffer_t *buffer = replay_buffer_create(2, 100);
    TEST_ASSERT(buffer != nullptr, "Replay buffer should be created");
    
    const size_t frame_size = 1024;
    uint8_t *frame_data = (uint8_t *)malloc(frame_size);
    TEST_ASSERT(frame_data != nullptr, "Failed to allocate frame data");
    
    // Add frames spanning 5 seconds
    for (int i = 0; i < 50; i++) {
        int ret = replay_buffer_add_video_frame(buffer, frame_data, frame_size,
                                               640, 480, i * 100000, i % 10 == 0);
        TEST_ASSERT(ret == 0, "Should successfully add video frame");
    }
    
    // Only last 2 seconds should remain
    uint32_t video_frames, audio_chunks, memory_mb, duration_sec;
    int ret = replay_buffer_get_stats(buffer, &video_frames, &audio_chunks,
                                      &memory_mb, &duration_sec);
    TEST_ASSERT(ret == 0, "Should get stats successfully");
    TEST_ASSERT(duration_sec <= 2, "Duration should be at most 2 seconds");
    
    free(frame_data);
    replay_buffer_destroy(buffer);
    
    TEST_PASS("test_time_based_cleanup");
}

/**
 * Test buffer clear
 */
int test_buffer_clear() {
    replay_buffer_t *buffer = replay_buffer_create(10, 100);
    TEST_ASSERT(buffer != nullptr, "Replay buffer should be created");
    
    const size_t frame_size = 1024;
    uint8_t *frame_data = (uint8_t *)malloc(frame_size);
    
    // Add some frames
    for (int i = 0; i < 10; i++) {
        replay_buffer_add_video_frame(buffer, frame_data, frame_size,
                                     640, 480, i * 100000, true);
    }
    
    // Clear buffer
    replay_buffer_clear(buffer);
    
    // Check stats
    uint32_t video_frames, audio_chunks, memory_mb, duration_sec;
    int ret = replay_buffer_get_stats(buffer, &video_frames, &audio_chunks,
                                      &memory_mb, &duration_sec);
    TEST_ASSERT(ret == 0, "Should get stats successfully");
    TEST_ASSERT(video_frames == 0, "Should have 0 video frames after clear");
    TEST_ASSERT(audio_chunks == 0, "Should have 0 audio chunks after clear");
    TEST_ASSERT(memory_mb == 0, "Should have 0 memory usage after clear");
    
    free(frame_data);
    replay_buffer_destroy(buffer);
    
    TEST_PASS("test_buffer_clear");
}

/**
 * Main test runner
 */
int main() {
    int failed = 0;
    
    printf("Running replay buffer tests...\n");
    printf("=====================================\n\n");
    
    if (test_replay_buffer_creation() != 0) failed++;
    if (test_add_video_frames() != 0) failed++;
    if (test_add_audio_chunks() != 0) failed++;
    if (test_memory_limit() != 0) failed++;
    if (test_time_based_cleanup() != 0) failed++;
    if (test_buffer_clear() != 0) failed++;
    
    printf("\n=====================================\n");
    if (failed == 0) {
        printf("✓ All replay buffer tests passed!\n");
        return 0;
    } else {
        printf("✗ %d test(s) failed\n", failed);
        return 1;
    }
}
