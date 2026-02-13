#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../../src/recording/recording_types.h"

// Simple test harness macros
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

// Test recording types definitions
int test_recording_types() {
    recording_info_t info = {0};
    
    info.recording_id = 1;
    info.video_codec = VIDEO_CODEC_H264;
    info.audio_codec = AUDIO_CODEC_OPUS;
    info.container = CONTAINER_MP4;
    info.preset = PRESET_BALANCED;
    
    TEST_ASSERT(info.recording_id == 1, "recording_id should be 1");
    TEST_ASSERT(info.video_codec == VIDEO_CODEC_H264, "video_codec should be H264");
    TEST_ASSERT(info.audio_codec == AUDIO_CODEC_OPUS, "audio_codec should be Opus");
    
    TEST_PASS("test_recording_types");
}

// Test video frame structure
int test_video_frame() {
    video_frame_t frame = {0};
    
    uint8_t dummy_data[1024];
    frame.data = dummy_data;
    frame.size = sizeof(dummy_data);
    frame.timestamp_us = 1000000;
    frame.frame_number = 1;
    
    TEST_ASSERT(frame.data != NULL, "frame data should not be null");
    TEST_ASSERT(frame.size == 1024, "frame size should be 1024");
    TEST_ASSERT(frame.timestamp_us == 1000000, "timestamp should be 1000000");
    
    TEST_PASS("test_video_frame");
}

// Test audio chunk structure
int test_audio_chunk() {
    audio_chunk_t chunk = {0};
    
    float dummy_samples[512];
    chunk.samples = dummy_samples;
    chunk.sample_count = 512;
    chunk.timestamp_us = 2000000;
    
    TEST_ASSERT(chunk.samples != NULL, "samples should not be null");
    TEST_ASSERT(chunk.sample_count == 512, "sample_count should be 512");
    TEST_ASSERT(chunk.timestamp_us == 2000000, "timestamp should be 2000000");
    
    TEST_PASS("test_audio_chunk");
}

int main() {
    int failed = 0;
    
    printf("Running recording types tests...\n");
    
    if (test_recording_types() != 0) failed++;
    if (test_video_frame() != 0) failed++;
    if (test_audio_chunk() != 0) failed++;
    
    if (failed == 0) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ %d test(s) failed\n", failed);
        return 1;
    }
}
