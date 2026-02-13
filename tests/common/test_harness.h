/*
 * test_harness.h - Common test utilities for RootStream tests
 *
 * Provides test runner, assertion macros, and common types
 * for unit and integration tests.
 */

#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Test result types */
typedef enum {
    TEST_PASS = 0,
    TEST_FAIL = 1,
    TEST_SKIP = 2,
} test_result_t;

/* Test case structure */
typedef struct {
    const char *name;
    test_result_t (*fn)(void);
} test_case_t;

/* Mock context structure for testing */
typedef struct {
    struct {
        int width;
        int height;
        uint32_t format;
    } current_frame;
    
    struct {
        int capture_drm;
        int capture_x11;
        int capture_dummy;
        int encode_nvenc;
        int encode_vaapi;
        int encode_x264;
        int encode_dummy;
        int audio_alsa;
        int audio_pulse;
        int audio_pipewire;
        int audio_dummy;
    } features;
    
    struct {
        char capture_name[64];
        char encoder_name[64];
        char audio_cap_name[64];
        char audio_play_name[64];
        char network_name[64];
        char discovery_name[64];
    } active_backend;
} rootstream_ctx_t;

/* Frame buffer structure for testing */
typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
} frame_buffer_t;

/* Run a suite of tests and report results */
int run_test_suite(const test_case_t *tests);

/* Assert macros */
#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            printf("  FAIL: %s != %s (line %d)\n", #a, #b, __LINE__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            printf("  FAIL: %s == %s (line %d)\n", #a, #b, __LINE__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define ASSERT_TRUE(x) \
    do { \
        if (!(x)) { \
            printf("  FAIL: %s is false (line %d)\n", #x, __LINE__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define ASSERT_FALSE(x) \
    do { \
        if ((x)) { \
            printf("  FAIL: %s is true (line %d)\n", #x, __LINE__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define ASSERT_NULL(x) \
    do { \
        if ((x) != NULL) { \
            printf("  FAIL: %s is not NULL (line %d)\n", #x, __LINE__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(x) \
    do { \
        if ((x) == NULL) { \
            printf("  FAIL: %s is NULL (line %d)\n", #x, __LINE__); \
            return TEST_FAIL; \
        } \
    } while(0)

#define ASSERT_STR_EQ(a, b) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            printf("  FAIL: %s != %s (line %d)\n", #a, #b, __LINE__); \
            return TEST_FAIL; \
        } \
    } while(0)

#endif /* TEST_HARNESS_H */
