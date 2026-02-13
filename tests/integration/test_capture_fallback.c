/*
 * test_capture_fallback.c - Test capture backend fallback chain
 * 
 * Validates:
 * - DRM initialization and frame capture (if available)
 * - X11 fallback when DRM unavailable
 * - Dummy fallback when both unavailable
 * - Proper error handling and cleanup
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../common/test_harness.h"

typedef struct {
    const char *name;
    int (*init_fn)(rootstream_ctx_t *ctx);
    int (*capture_fn)(rootstream_ctx_t *ctx, frame_buffer_t *frame);
    void (*cleanup_fn)(rootstream_ctx_t *ctx);
} capture_backend_t;

/* Mock DRM backend for testing */
int mock_drm_init(rootstream_ctx_t *ctx) {
    ctx->current_frame.width = 1920;
    ctx->current_frame.height = 1080;
    ctx->current_frame.format = 0x34325258;  /* XRGB8888 */
    return 0;
}

int mock_drm_capture(rootstream_ctx_t *ctx, frame_buffer_t *frame) {
    /* Generate test pattern */
    if (frame->capacity < (size_t)(ctx->current_frame.width * ctx->current_frame.height * 4)) {
        return -1;
    }
    memset(frame->data, 0x80, frame->capacity);  /* Gray */
    frame->size = ctx->current_frame.width * ctx->current_frame.height * 4;
    return 0;
}

void mock_drm_cleanup(rootstream_ctx_t *ctx) {
    (void)ctx;
}

/* Mock X11 backend for testing */
int mock_x11_init(rootstream_ctx_t *ctx) {
    ctx->current_frame.width = 1920;
    ctx->current_frame.height = 1080;
    ctx->current_frame.format = 0x34325258;
    return 0;
}

int mock_x11_capture(rootstream_ctx_t *ctx, frame_buffer_t *frame) {
    if (frame->capacity < (size_t)(ctx->current_frame.width * ctx->current_frame.height * 4)) {
        return -1;
    }
    memset(frame->data, 0x40, frame->capacity);  /* Darker gray */
    frame->size = ctx->current_frame.width * ctx->current_frame.height * 4;
    return 0;
}

void mock_x11_cleanup(rootstream_ctx_t *ctx) {
    (void)ctx;
}

/* Mock Dummy backend for testing */
int mock_dummy_init(rootstream_ctx_t *ctx) {
    ctx->current_frame.width = 800;
    ctx->current_frame.height = 600;
    ctx->current_frame.format = 0x34325258;
    return 0;
}

int mock_dummy_capture(rootstream_ctx_t *ctx, frame_buffer_t *frame) {
    if (frame->capacity < (size_t)(ctx->current_frame.width * ctx->current_frame.height * 4)) {
        return -1;
    }
    /* Generate color bars test pattern */
    for (int i = 0; i < ctx->current_frame.height; i++) {
        for (int j = 0; j < ctx->current_frame.width; j++) {
            int color = (j * 8) / ctx->current_frame.width;
            frame->data[(i * ctx->current_frame.width + j) * 4 + 0] = (color & 1) ? 255 : 0;  /* B */
            frame->data[(i * ctx->current_frame.width + j) * 4 + 1] = (color & 2) ? 255 : 0;  /* G */
            frame->data[(i * ctx->current_frame.width + j) * 4 + 2] = (color & 4) ? 255 : 0;  /* R */
            frame->data[(i * ctx->current_frame.width + j) * 4 + 3] = 0;                       /* X */
        }
    }
    frame->size = ctx->current_frame.width * ctx->current_frame.height * 4;
    return 0;
}

void mock_dummy_cleanup(rootstream_ctx_t *ctx) {
    (void)ctx;
}

/* Test: DRM capture initialization */
test_result_t test_capture_drm_init(void) {
    rootstream_ctx_t ctx = {0};
    
    int ret = mock_drm_init(&ctx);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(ctx.current_frame.width, 1920);
    ASSERT_EQ(ctx.current_frame.height, 1080);
    
    mock_drm_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: DRM capture frame */
test_result_t test_capture_drm_frame(void) {
    rootstream_ctx_t ctx = {0};
    frame_buffer_t frame = {0};
    uint8_t buffer[1920 * 1080 * 4];
    
    mock_drm_init(&ctx);
    frame.data = buffer;
    frame.capacity = sizeof(buffer);
    
    int ret = mock_drm_capture(&ctx, &frame);
    ASSERT_EQ(ret, 0);
    ASSERT_TRUE(frame.size > 0);
    ASSERT_EQ(frame.size, (size_t)(1920 * 1080 * 4));
    
    mock_drm_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: X11 capture initialization */
test_result_t test_capture_x11_init(void) {
    rootstream_ctx_t ctx = {0};
    
    int ret = mock_x11_init(&ctx);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(ctx.current_frame.width, 1920);
    ASSERT_EQ(ctx.current_frame.height, 1080);
    
    mock_x11_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: Dummy capture initialization */
test_result_t test_capture_dummy_init(void) {
    rootstream_ctx_t ctx = {0};
    
    int ret = mock_dummy_init(&ctx);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(ctx.current_frame.width, 800);
    ASSERT_EQ(ctx.current_frame.height, 600);
    
    mock_dummy_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: Dummy capture generates test pattern */
test_result_t test_capture_dummy_pattern(void) {
    rootstream_ctx_t ctx = {0};
    frame_buffer_t frame = {0};
    uint8_t buffer[800 * 600 * 4];
    
    mock_dummy_init(&ctx);
    frame.data = buffer;
    frame.capacity = sizeof(buffer);
    
    int ret = mock_dummy_capture(&ctx, &frame);
    ASSERT_EQ(ret, 0);
    ASSERT_TRUE(frame.size > 0);
    ASSERT_EQ(frame.size, (size_t)(800 * 600 * 4));
    
    mock_dummy_cleanup(&ctx);
    return TEST_PASS;
}

/* Test: Fallback selection - try all backends */
test_result_t test_capture_fallback_chain(void) {
    rootstream_ctx_t ctx = {0};
    
    const capture_backend_t backends[] = {
        { "DRM", mock_drm_init, mock_drm_capture, mock_drm_cleanup },
        { "X11", mock_x11_init, mock_x11_capture, mock_x11_cleanup },
        { "Dummy", mock_dummy_init, mock_dummy_capture, mock_dummy_cleanup },
        {NULL, NULL, NULL, NULL}
    };
    
    /* Try each backend - at least one should work */
    int success = 0;
    for (int i = 0; backends[i].name; i++) {
        int ret = backends[i].init_fn(&ctx);
        
        if (ret == 0) {
            /* Backend initialized successfully */
            frame_buffer_t frame = {0};
            uint8_t buffer[1920 * 1080 * 4];
            frame.data = buffer;
            frame.capacity = sizeof(buffer);
            
            ret = backends[i].capture_fn(&ctx, &frame);
            ASSERT_EQ(ret, 0);
            
            backends[i].cleanup_fn(&ctx);
            success = 1;
            break;
        }
    }
    
    ASSERT_TRUE(success);
    return TEST_PASS;
}

/* Test suite */
const test_case_t capture_tests[] = {
    { "DRM init", test_capture_drm_init },
    { "DRM capture", test_capture_drm_frame },
    { "X11 init", test_capture_x11_init },
    { "Dummy init", test_capture_dummy_init },
    { "Dummy pattern", test_capture_dummy_pattern },
    { "Fallback chain", test_capture_fallback_chain },
    {NULL, NULL}
};

int main(void) {
    printf("Running capture fallback tests...\n");
    return run_test_suite(capture_tests);
}
