#include "../src/recording/disk_manager.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>

// Test macros
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

// Test directory creation and initialization
int test_disk_manager_init() {
    DiskManager dm;
    
    const char *test_dir = "/tmp/rootstream_test_recordings";
    
    // Clean up any existing test directory
    system("rm -rf /tmp/rootstream_test_recordings");
    
    int result = dm.init(test_dir, 1000);  // 1GB limit
    
    TEST_ASSERT(result == 0, "disk manager initialization should succeed");
    
    // Verify directory was created
    struct stat st;
    TEST_ASSERT(stat(test_dir, &st) == 0, "test directory should exist");
    TEST_ASSERT(S_ISDIR(st.st_mode), "test path should be a directory");
    
    // Clean up
    dm.cleanup();
    system("rm -rf /tmp/rootstream_test_recordings");
    
    TEST_PASS("test_disk_manager_init");
}

// Test disk space queries
int test_disk_space_queries() {
    DiskManager dm;
    const char *test_dir = "/tmp/rootstream_test_recordings";
    
    system("rm -rf /tmp/rootstream_test_recordings");
    
    if (dm.init(test_dir, 1000) != 0) {
        fprintf(stderr, "FAIL: initialization failed\n");
        return -1;
    }
    
    uint64_t free_space = dm.get_free_space_mb();
    uint64_t used_space = dm.get_used_space_mb();
    float usage_percent = dm.get_usage_percent();
    
    TEST_ASSERT(free_space > 0, "free space should be positive");
    TEST_ASSERT(usage_percent >= 0.0f && usage_percent <= 100.0f, 
                "usage percent should be between 0 and 100");
    
    printf("  Free space: %lu MB\n", free_space);
    printf("  Used space: %lu MB\n", used_space);
    printf("  Usage: %.1f%%\n", usage_percent);
    
    // Clean up
    dm.cleanup();
    system("rm -rf /tmp/rootstream_test_recordings");
    
    TEST_PASS("test_disk_space_queries");
}

// Test filename generation
int test_filename_generation() {
    DiskManager dm;
    const char *test_dir = "/tmp/rootstream_test_recordings";
    
    system("rm -rf /tmp/rootstream_test_recordings");
    
    if (dm.init(test_dir, 1000) != 0) {
        fprintf(stderr, "FAIL: initialization failed\n");
        return -1;
    }
    
    // Generate filename without game name
    std::string filename1 = dm.generate_filename(nullptr);
    TEST_ASSERT(!filename1.empty(), "filename should not be empty");
    TEST_ASSERT(filename1.find("recording_") == 0, "filename should start with 'recording_'");
    TEST_ASSERT(filename1.find(".mp4") != std::string::npos, "filename should end with .mp4");
    
    printf("  Generated filename: %s\n", filename1.c_str());
    
    // Generate filename with game name
    std::string filename2 = dm.generate_filename("TestGame");
    TEST_ASSERT(!filename2.empty(), "filename should not be empty");
    TEST_ASSERT(filename2.find("TestGame_") == 0, "filename should start with 'TestGame_'");
    TEST_ASSERT(filename2.find(".mp4") != std::string::npos, "filename should end with .mp4");
    
    printf("  Generated filename with game: %s\n", filename2.c_str());
    
    // Clean up
    dm.cleanup();
    system("rm -rf /tmp/rootstream_test_recordings");
    
    TEST_PASS("test_filename_generation");
}

// Test file cleanup
int test_file_cleanup() {
    DiskManager dm;
    const char *test_dir = "/tmp/rootstream_test_recordings";
    
    system("rm -rf /tmp/rootstream_test_recordings");
    
    if (dm.init(test_dir, 1000) != 0) {
        fprintf(stderr, "FAIL: initialization failed\n");
        return -1;
    }
    
    // Create some test files
    char filepath[1024];
    for (int i = 0; i < 5; i++) {
        snprintf(filepath, sizeof(filepath), "%s/test_recording_%d.mp4", test_dir, i);
        FILE *f = fopen(filepath, "w");
        if (f) {
            fprintf(f, "test data\n");
            fclose(f);
        }
    }
    
    // Test cleanup_directory
    int count = dm.cleanup_directory();
    TEST_ASSERT(count == 5, "should have cleaned up 5 files");
    
    // Verify files are gone
    for (int i = 0; i < 5; i++) {
        snprintf(filepath, sizeof(filepath), "%s/test_recording_%d.mp4", test_dir, i);
        struct stat st;
        TEST_ASSERT(stat(filepath, &st) != 0, "file should not exist after cleanup");
    }
    
    // Clean up
    dm.cleanup();
    system("rm -rf /tmp/rootstream_test_recordings");
    
    TEST_PASS("test_file_cleanup");
}

int main() {
    int failed = 0;
    
    printf("Running disk manager tests...\n");
    
    if (test_disk_manager_init() != 0) failed++;
    if (test_disk_space_queries() != 0) failed++;
    if (test_filename_generation() != 0) failed++;
    if (test_file_cleanup() != 0) failed++;
    
    if (failed == 0) {
        printf("\n✓ All disk manager tests passed!\n");
        return 0;
    } else {
        printf("\n✗ %d test(s) failed\n", failed);
        return 1;
    }
}
