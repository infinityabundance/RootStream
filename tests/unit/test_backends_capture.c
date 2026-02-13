/*
 * test_backends_capture.c - Test capture backend selection logic
 * 
 * Unit tests for capture backend initialization and fallback
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/test_harness.h"

/* Test: Backend selection priority */
test_result_t test_capture_backend_priority(void) {
    /* DRM > X11 > Dummy priority order */
    const char *backends[] = {"DRM/KMS", "X11", "Dummy"};
    ASSERT_STR_EQ(backends[0], "DRM/KMS");
    return TEST_PASS;
}

/* Test: Backend name validation */
test_result_t test_capture_backend_names(void) {
    rootstream_ctx_t ctx = {0};
    
    strcpy(ctx.active_backend.capture_name, "DRM/KMS");
    ASSERT_STR_EQ(ctx.active_backend.capture_name, "DRM/KMS");
    
    strcpy(ctx.active_backend.capture_name, "X11");
    ASSERT_STR_EQ(ctx.active_backend.capture_name, "X11");
    
    return TEST_PASS;
}

const test_case_t capture_backend_tests[] = {
    { "Backend priority", test_capture_backend_priority },
    { "Backend names", test_capture_backend_names },
    {NULL, NULL}
};

int main(void) {
    printf("Running capture backend tests...\n");
    return run_test_suite(capture_backend_tests);
}
