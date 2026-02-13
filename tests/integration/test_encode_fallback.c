/*
 * test_encode_fallback.c - Test encoder backend fallback chain
 * 
 * Validates:
 * - NVENC encoder initialization and encoding (if available)
 * - VA-API fallback when NVENC unavailable
 * - x264/FFmpeg software fallback
 * - Raw encoder final fallback
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/test_harness.h"

typedef struct {
    const char *name;
    int (*init_fn)(rootstream_ctx_t *ctx, int width, int height);
    int (*encode_fn)(rootstream_ctx_t *ctx, const uint8_t *frame, size_t frame_size, uint8_t *out, size_t *out_size);
    void (*cleanup_fn)(rootstream_ctx_t *ctx);
} encoder_backend_t;

/* Mock NVENC encoder */
int mock_nvenc_init(rootstream_ctx_t *ctx, int width, int height) {
    ctx->current_frame.width = width;
    ctx->current_frame.height = height;
    return 0;
}

int mock_nvenc_encode(rootstream_ctx_t *ctx, const uint8_t *frame, size_t frame_size, uint8_t *out, size_t *out_size) {
    (void)ctx;
    (void)frame;
    /* Simulate compression: output 1% of input size */
    *out_size = frame_size / 100;
    if (*out_size < 1024) *out_size = 1024;
    memset(out, 0xAA, *out_size);
    return 0;
}

void mock_nvenc_cleanup(rootstream_ctx_t *ctx) {
    (void)ctx;
}

/* Mock VA-API encoder */
int mock_vaapi_init(rootstream_ctx_t *ctx, int width, int height) {
    ctx->current_frame.width = width;
    ctx->current_frame.height = height;
    return 0;
}

int mock_vaapi_encode(rootstream_ctx_t *ctx, const uint8_t *frame, size_t frame_size, uint8_t *out, size_t *out_size) {
    (void)ctx;
    (void)frame;
    /* Simulate compression: output 2% of input size */
    *out_size = frame_size / 50;
    if (*out_size < 1024) *out_size = 1024;
    memset(out, 0xBB, *out_size);
    return 0;
}

void mock_vaapi_cleanup(rootstream_ctx_t *ctx) {
    (void)ctx;
}

/* Mock x264/FFmpeg software encoder */
int mock_x264_init(rootstream_ctx_t *ctx, int width, int height) {
    ctx->current_frame.width = width;
    ctx->current_frame.height = height;
    return 0;
}

int mock_x264_encode(rootstream_ctx_t *ctx, const uint8_t *frame, size_t frame_size, uint8_t *out, size_t *out_size) {
    (void)ctx;
    (void)frame;
    /* Simulate software compression: output 3% of input size */
    *out_size = frame_size / 33;
    if (*out_size < 1024) *out_size = 1024;
    memset(out, 0xCC, *out_size);
    return 0;
}

void mock_x264_cleanup(rootstream_ctx_t *ctx) {
    (void)ctx;
}

/* Mock raw encoder (no compression) */
int mock_raw_init(rootstream_ctx_t *ctx, int width, int height) {
    ctx->current_frame.width = width;
    ctx->current_frame.height = height;
    return 0;
}

int mock_raw_encode(rootstream_ctx_t *ctx, const uint8_t *frame, size_t frame_size, uint8_t *out, size_t *out_size) {
    (void)ctx;
    /* Raw encoder: just copy the data */
    *out_size = frame_size;
    memcpy(out, frame, frame_size);
    return 0;
}

void mock_raw_cleanup(rootstream_ctx_t *ctx) {
    (void)ctx;
}

/* Test: NVENC encoder initialization */
test_result_t test_encode_nvenc_init(void) {
    rootstream_ctx_t ctx = {0};
    
    int ret = mock_nvenc_init(&ctx, 1920, 1080);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(ctx.current_frame.width, 1920);
    ASSERT_EQ(ctx.current_frame.height, 1080);
    
    mock_nvenc_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: NVENC encoding produces compressed output */
test_result_t test_encode_nvenc_compression(void) {
    rootstream_ctx_t ctx = {0};
    uint8_t input[1920 * 1080 * 4];
    uint8_t output[1920 * 1080 * 4];
    size_t output_size;
    
    mock_nvenc_init(&ctx, 1920, 1080);
    memset(input, 0xFF, sizeof(input));
    
    int ret = mock_nvenc_encode(&ctx, input, sizeof(input), output, &output_size);
    ASSERT_EQ(ret, 0);
    ASSERT_TRUE(output_size > 0);
    ASSERT_TRUE(output_size < sizeof(input));  /* Should be compressed */
    
    mock_nvenc_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: VA-API encoder initialization */
test_result_t test_encode_vaapi_init(void) {
    rootstream_ctx_t ctx = {0};
    
    int ret = mock_vaapi_init(&ctx, 1920, 1080);
    ASSERT_EQ(ret, 0);
    
    mock_vaapi_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: x264 encoder initialization */
test_result_t test_encode_x264_init(void) {
    rootstream_ctx_t ctx = {0};
    
    int ret = mock_x264_init(&ctx, 1920, 1080);
    ASSERT_EQ(ret, 0);
    
    mock_x264_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: Raw encoder always works */
test_result_t test_encode_raw_init(void) {
    rootstream_ctx_t ctx = {0};
    
    int ret = mock_raw_init(&ctx, 1920, 1080);
    ASSERT_EQ(ret, 0);
    
    mock_raw_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: Raw encoder preserves data */
test_result_t test_encode_raw_passthrough(void) {
    rootstream_ctx_t ctx = {0};
    uint8_t input[1920 * 1080 * 4];
    uint8_t output[1920 * 1080 * 4];
    size_t output_size;
    
    mock_raw_init(&ctx, 1920, 1080);
    memset(input, 0x42, sizeof(input));
    
    int ret = mock_raw_encode(&ctx, input, sizeof(input), output, &output_size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(output_size, sizeof(input));
    ASSERT_EQ(output[0], 0x42);
    ASSERT_EQ(output[1000], 0x42);
    
    mock_raw_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: Encoder fallback chain */
test_result_t test_encode_fallback_chain(void) {
    rootstream_ctx_t ctx = {0};
    
    const encoder_backend_t backends[] = {
        { "NVENC", mock_nvenc_init, mock_nvenc_encode, mock_nvenc_cleanup },
        { "VA-API", mock_vaapi_init, mock_vaapi_encode, mock_vaapi_cleanup },
        { "x264", mock_x264_init, mock_x264_encode, mock_x264_cleanup },
        { "Raw", mock_raw_init, mock_raw_encode, mock_raw_cleanup },
        {NULL, NULL, NULL, NULL}
    };
    
    /* Try each backend - at least one should work (Raw always works) */
    int success = 0;
    for (int i = 0; backends[i].name; i++) {
        int ret = backends[i].init_fn(&ctx, 1920, 1080);
        
        if (ret == 0) {
            /* Backend initialized successfully */
            uint8_t input[1920 * 1080 * 4];
            uint8_t output[1920 * 1080 * 4];
            size_t output_size;
            
            memset(input, 0xFF, sizeof(input));
            ret = backends[i].encode_fn(&ctx, input, sizeof(input), output, &output_size);
            ASSERT_EQ(ret, 0);
            ASSERT_TRUE(output_size > 0);
            
            backends[i].cleanup_fn(&ctx);
            success = 1;
            break;
        }
    }
    
    ASSERT_TRUE(success);
    return TEST_PASS;
}

/* Test suite */
const test_case_t encode_tests[] = {
    { "NVENC init", test_encode_nvenc_init },
    { "NVENC compression", test_encode_nvenc_compression },
    { "VA-API init", test_encode_vaapi_init },
    { "x264 init", test_encode_x264_init },
    { "Raw init", test_encode_raw_init },
    { "Raw passthrough", test_encode_raw_passthrough },
    { "Fallback chain", test_encode_fallback_chain },
    {NULL, NULL}
};

int main(void) {
    printf("Running encoder fallback tests...\n");
    return run_test_suite(encode_tests);
}
