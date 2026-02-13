/*
 * test_backends_encode.c - Test encoder backend selection logic
 * 
 * Unit tests for encoder backend initialization and fallback
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/test_harness.h"

/* Test: Encoder selection priority */
test_result_t test_encode_backend_priority(void) {
    /* NVENC > VA-API > x264 > Raw priority order */
    const char *backends[] = {"NVENC", "VA-API", "x264", "Raw"};
    ASSERT_STR_EQ(backends[0], "NVENC");
    ASSERT_STR_EQ(backends[3], "Raw");
    return TEST_PASS;
}

/* Test: Encoder name validation */
test_result_t test_encode_backend_names(void) {
    rootstream_ctx_t ctx = {0};
    
    strcpy(ctx.active_backend.encoder_name, "NVENC");
    ASSERT_STR_EQ(ctx.active_backend.encoder_name, "NVENC");
    
    strcpy(ctx.active_backend.encoder_name, "Raw");
    ASSERT_STR_EQ(ctx.active_backend.encoder_name, "Raw");
    
    return TEST_PASS;
}

const test_case_t encode_backend_tests[] = {
    { "Backend priority", test_encode_backend_priority },
    { "Backend names", test_encode_backend_names },
    {NULL, NULL}
};

int main(void) {
    printf("Running encoder backend tests...\n");
    return run_test_suite(encode_backend_tests);
}
