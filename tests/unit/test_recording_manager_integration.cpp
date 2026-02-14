/**
 * Test for RecordingManager replay buffer integration
 * 
 * This test verifies:
 * 1. Enabling/disabling replay buffer
 * 2. Replay buffer integration with RecordingManager
 * 3. Saving replay buffer through RecordingManager
 * 4. Metadata methods (chapter markers, game name, audio tracks)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../src/recording/recording_manager.h"

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
 * Test replay buffer enable/disable
 */
int test_replay_buffer_enable_disable() {
    RecordingManager manager;
    
    // Initialize manager
    int ret = manager.init("/tmp/test_recordings");
    TEST_ASSERT(ret == 0, "RecordingManager init should succeed");
    
    // Enable replay buffer
    ret = manager.enable_replay_buffer(30, 100);
    TEST_ASSERT(ret == 0, "Should enable replay buffer");
    
    // Try to enable again (should fail)
    ret = manager.enable_replay_buffer(30, 100);
    TEST_ASSERT(ret != 0, "Should not enable replay buffer twice");
    
    // Disable replay buffer
    ret = manager.disable_replay_buffer();
    TEST_ASSERT(ret == 0, "Should disable replay buffer");
    
    // Disable again (should succeed but do nothing)
    ret = manager.disable_replay_buffer();
    TEST_ASSERT(ret == 0, "Should handle double disable gracefully");
    
    manager.cleanup();
    
    TEST_PASS("test_replay_buffer_enable_disable");
}

/**
 * Test metadata methods
 */
int test_metadata_methods() {
    RecordingManager manager;
    manager.init("/tmp/test_recordings");
    
    // Set game name
    int ret = manager.set_game_name("Test Game");
    TEST_ASSERT(ret == 0, "Should set game name");
    
    // Add audio track
    ret = manager.add_audio_track("Game Audio", 2, 48000);
    TEST_ASSERT(ret == 0, "Should add audio track");
    
    ret = manager.add_audio_track("Microphone", 1, 48000);
    TEST_ASSERT(ret == 0, "Should add second audio track");
    
    // Start recording to test chapter markers
    ret = manager.start_recording(PRESET_BALANCED, "Test Game");
    TEST_ASSERT(ret == 0, "Should start recording");
    
    // Add chapter marker
    ret = manager.add_chapter_marker("Level 1", "Starting first level");
    TEST_ASSERT(ret == 0, "Should add chapter marker");
    
    // Stop recording
    ret = manager.stop_recording();
    TEST_ASSERT(ret == 0, "Should stop recording");
    
    manager.cleanup();
    
    TEST_PASS("test_metadata_methods");
}

/**
 * Test recording with different container formats
 */
int test_container_format_selection() {
    RecordingManager manager;
    manager.init("/tmp/test_recordings");
    
    // Test PRESET_FAST (should use MP4)
    int ret = manager.start_recording(PRESET_FAST, "TestGame_Fast");
    TEST_ASSERT(ret == 0, "Should start recording with FAST preset");
    
    const recording_info_t *info = manager.get_active_recording();
    TEST_ASSERT(info != nullptr, "Should have active recording");
    TEST_ASSERT(info->container == CONTAINER_MP4, "FAST preset should use MP4");
    
    manager.stop_recording();
    
    // Test PRESET_HIGH_QUALITY (should use Matroska)
    ret = manager.start_recording(PRESET_HIGH_QUALITY, "TestGame_High");
    TEST_ASSERT(ret == 0, "Should start recording with HIGH_QUALITY preset");
    
    info = manager.get_active_recording();
    TEST_ASSERT(info != nullptr, "Should have active recording");
    TEST_ASSERT(info->container == CONTAINER_MATROSKA, "HIGH_QUALITY preset should use Matroska");
    
    manager.stop_recording();
    
    manager.cleanup();
    
    TEST_PASS("test_container_format_selection");
}

/**
 * Test error handling
 */
int test_error_handling() {
    RecordingManager manager;
    manager.init("/tmp/test_recordings");
    
    // Try to add chapter marker without recording
    int ret = manager.add_chapter_marker("Test", "Description");
    TEST_ASSERT(ret != 0, "Should fail to add chapter marker without recording");
    
    // Try to save replay buffer without enabling it
    ret = manager.save_replay_buffer("test.mp4", 10);
    TEST_ASSERT(ret != 0, "Should fail to save replay buffer when not enabled");
    
    // Try to start recording twice
    ret = manager.start_recording(PRESET_BALANCED, "Game");
    TEST_ASSERT(ret == 0, "Should start recording");
    
    ret = manager.start_recording(PRESET_BALANCED, "Game");
    TEST_ASSERT(ret != 0, "Should fail to start recording twice");
    
    manager.stop_recording();
    manager.cleanup();
    
    TEST_PASS("test_error_handling");
}

/**
 * Test output directory configuration
 */
int test_output_directory() {
    RecordingManager manager;
    
    // Test with custom directory
    int ret = manager.init("/tmp/custom_recordings");
    TEST_ASSERT(ret == 0, "Should initialize with custom directory");
    
    // Set different directory
    ret = manager.set_output_directory("/tmp/another_dir");
    TEST_ASSERT(ret == 0, "Should set output directory");
    
    manager.cleanup();
    
    TEST_PASS("test_output_directory");
}

/**
 * Test storage configuration
 */
int test_storage_configuration() {
    RecordingManager manager;
    manager.init("/tmp/test_recordings");
    
    // Set max storage
    int ret = manager.set_max_storage(5000); // 5GB
    TEST_ASSERT(ret == 0, "Should set max storage");
    
    // Enable auto-cleanup
    ret = manager.set_auto_cleanup(true, 85);
    TEST_ASSERT(ret == 0, "Should enable auto-cleanup");
    
    manager.cleanup();
    
    TEST_PASS("test_storage_configuration");
}

/**
 * Main test runner
 */
int main() {
    int failed = 0;
    
    printf("Running RecordingManager integration tests...\n");
    printf("=====================================\n\n");
    
    if (test_replay_buffer_enable_disable() != 0) failed++;
    if (test_metadata_methods() != 0) failed++;
    if (test_container_format_selection() != 0) failed++;
    if (test_error_handling() != 0) failed++;
    if (test_output_directory() != 0) failed++;
    if (test_storage_configuration() != 0) failed++;
    
    printf("\n=====================================\n");
    if (failed == 0) {
        printf("✓ All RecordingManager integration tests passed!\n");
        return 0;
    } else {
        printf("✗ %d test(s) failed\n", failed);
        return 1;
    }
}
