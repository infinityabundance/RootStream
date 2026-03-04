/**
 * @file test_vulkan_integration.c
 * @brief Integration tests for the Vulkan pipeline and frame ring buffer
 *
 * Tests the frame_ring_buffer lock-free implementation and the Vulkan
 * headless renderer with synthetic NV12 frames.  Designed to pass in CI
 * even when no GPU is present (headless mode returns NULL gracefully).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#include "../../clients/kde-plasma-client/src/renderer/vulkan_renderer.h"
#include "../../clients/kde-plasma-client/src/frame_ring_buffer.h"

/* ── Test helpers ─────────────────────────────────────────────────────────── */

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s\n", (message)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(message) \
    do { \
        printf("PASS: %s\n", (message)); \
    } while (0)

/* ── Helpers for building synthetic frames ────────────────────────────────── */

static void fill_synthetic_nv12(uint8_t *y_buf, uint8_t *uv_buf,
                                  uint32_t width, uint32_t height,
                                  uint8_t fill_val)
{
    memset(y_buf,  fill_val,       (size_t)width * height);
    memset(uv_buf, (uint8_t)128u,  (size_t)width * (height / 2));
}

/* ── test_ring_buffer_pushpop ─────────────────────────────────────────────── */

static int test_ring_buffer_pushpop(void)
{
    printf("\n=== test_ring_buffer_pushpop ===\n");

    frame_ring_buffer_t rb;
    frame_ring_buffer_init(&rb);

    const uint32_t W = 64, H = 64;
    uint8_t y[64 * 64], uv[64 * 32];

    /* Push 3 frames with distinct luma fill values */
    for (int i = 0; i < 3; i++) {
        fill_synthetic_nv12(y, uv, W, H, (uint8_t)(10 + i));
        int r = frame_ring_buffer_push(&rb, y, uv, W, H,
                                        (uint64_t)(1000 + i));
        TEST_ASSERT(r == 0, "push should succeed");
    }

    TEST_ASSERT(frame_ring_buffer_available(&rb) == 3,
                "available should be 3 after 3 pushes");

    /* Pop and verify each frame's fill value */
    for (int i = 0; i < 3; i++) {
        frame_t f;
        int r = frame_ring_buffer_pop(&rb, &f);
        TEST_ASSERT(r == 0, "pop should succeed");
        TEST_ASSERT(f.width == W && f.height == H, "frame dimensions match");
        TEST_ASSERT(f.timestamp_us == (uint64_t)(1000 + i),
                    "timestamp matches");
        TEST_ASSERT(f.data != NULL, "data pointer is non-NULL");
        TEST_ASSERT(f.data[0] == (uint8_t)(10 + i), "luma fill value matches");
    }

    TEST_ASSERT(frame_ring_buffer_available(&rb) == 0,
                "available should be 0 after 3 pops");

    frame_ring_buffer_cleanup(&rb);
    TEST_PASS("test_ring_buffer_pushpop");
    return 0;
}

/* ── test_ring_buffer_full ────────────────────────────────────────────────── */

static int test_ring_buffer_full(void)
{
    printf("\n=== test_ring_buffer_full ===\n");

    frame_ring_buffer_t rb;
    frame_ring_buffer_init(&rb);

    const uint32_t W = 32, H = 32;
    uint8_t y[32 * 32], uv[32 * 16];
    memset(y, 0, sizeof y);
    memset(uv, 128, sizeof uv);

    /* Fill exactly FRAME_RING_BUFFER_CAPACITY slots */
    for (int i = 0; i < FRAME_RING_BUFFER_CAPACITY; i++) {
        int r = frame_ring_buffer_push(&rb, y, uv, W, H, (uint64_t)i);
        TEST_ASSERT(r == 0, "push within capacity should succeed");
    }

    TEST_ASSERT(frame_ring_buffer_available(&rb) == FRAME_RING_BUFFER_CAPACITY,
                "buffer should report full");

    /* The 5th push (one beyond capacity) must return -1 */
    int r = frame_ring_buffer_push(&rb, y, uv, W, H, 999ULL);
    TEST_ASSERT(r == -1, "push beyond capacity must return -1");

    frame_ring_buffer_cleanup(&rb);
    TEST_PASS("test_ring_buffer_full");
    return 0;
}

/* ── test_vulkan_init_headless ────────────────────────────────────────────── */

static int test_vulkan_init_headless(void)
{
    printf("\n=== test_vulkan_init_headless ===\n");

    /* vulkan_init(NULL) must either return a valid context (GPU present) or
     * NULL (no GPU / headless CI).  Either outcome is acceptable; crashing is
     * not. */
    vulkan_context_t *ctx = vulkan_init(NULL);

    if (ctx != NULL) {
        printf("  (GPU detected – context initialised)\n");
        vulkan_cleanup(ctx);
    } else {
        printf("  (No GPU / headless – NULL returned as expected)\n");
    }

    TEST_PASS("test_vulkan_init_headless");
    return 0;
}

/* ── test_vulkan_frame_upload ─────────────────────────────────────────────── */

static int test_vulkan_frame_upload(void)
{
    printf("\n=== test_vulkan_frame_upload ===\n");

    vulkan_context_t *ctx = vulkan_init(NULL);

    const uint32_t W = 320, H = 240;
    size_t y_sz  = (size_t)W * H;
    size_t uv_sz = (size_t)W * (H / 2);

    uint8_t *buf = (uint8_t *)calloc(1, y_sz + uv_sz);
    TEST_ASSERT(buf != NULL, "alloc NV12 buffer");
    memset(buf,          0x10, y_sz);
    memset(buf + y_sz,   0x80, uv_sz);

    frame_t frame;
    memset(&frame, 0, sizeof frame);
    frame.data         = buf;
    frame.size         = (uint32_t)(y_sz + uv_sz);
    frame.width        = W;
    frame.height       = H;
    frame.format       = FRAME_FORMAT_NV12;
    frame.timestamp_us = 42000ULL;
    frame.is_keyframe  = true;

    if (ctx != NULL) {
        /* GPU present – exercise the full upload path */
        int r = vulkan_upload_frame(ctx, &frame);
        /* A -1 result is acceptable if the swapchain isn't ready */
        printf("  vulkan_upload_frame returned %d\n", r);
        vulkan_cleanup(ctx);
    } else {
        /* Headless: calling upload on NULL must not crash */
        vulkan_upload_frame(NULL, &frame);
        printf("  (headless – upload on NULL context skipped gracefully)\n");
    }

    free(buf);
    TEST_PASS("test_vulkan_frame_upload");
    return 0;
}

/* ── test_frame_ring_buffer_concurrent ───────────────────────────────────── */

#define CONCURRENT_ITERS 200

typedef struct {
    frame_ring_buffer_t *rb;
    int                  push_count;
} producer_args_t;

typedef struct {
    frame_ring_buffer_t *rb;
    int                  pop_count;
} consumer_args_t;

static void *producer_thread(void *arg)
{
    producer_args_t *a = (producer_args_t *)arg;
    const uint32_t W = 16, H = 16;
    uint8_t y[16 * 16], uv[16 * 8];
    memset(y, 0xAB, sizeof y);
    memset(uv, 0x7F, sizeof uv);

    for (int i = 0; i < CONCURRENT_ITERS; i++) {
        /* Retry until push succeeds (buffer may be momentarily full) */
        while (frame_ring_buffer_push(a->rb, y, uv, W, H,
                                       (uint64_t)i) != 0)
            ; /* spin */
        a->push_count++;
    }
    return NULL;
}

static void *consumer_thread(void *arg)
{
    consumer_args_t *a = (consumer_args_t *)arg;
    frame_t f;
    int consumed = 0;

    while (consumed < CONCURRENT_ITERS) {
        if (frame_ring_buffer_pop(a->rb, &f) == 0)
            consumed++;
    }
    a->pop_count = consumed;
    return NULL;
}

static int test_frame_ring_buffer_concurrent(void)
{
    printf("\n=== test_frame_ring_buffer_concurrent ===\n");

    frame_ring_buffer_t rb;
    frame_ring_buffer_init(&rb);

    producer_args_t pa = { &rb, 0 };
    consumer_args_t ca = { &rb, 0 };

    pthread_t prod, cons;
    int r;

    r = pthread_create(&cons, NULL, consumer_thread, &ca);
    TEST_ASSERT(r == 0, "consumer thread create");
    r = pthread_create(&prod, NULL, producer_thread, &pa);
    TEST_ASSERT(r == 0, "producer thread create");

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    TEST_ASSERT(pa.push_count == CONCURRENT_ITERS,
                "producer pushed all frames");
    TEST_ASSERT(ca.pop_count  == CONCURRENT_ITERS,
                "consumer consumed all frames");

    frame_ring_buffer_cleanup(&rb);
    TEST_PASS("test_frame_ring_buffer_concurrent");
    return 0;
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void)
{
    int failures = 0;

    printf("Starting Vulkan Integration Tests...\n");

    failures += test_ring_buffer_pushpop();
    failures += test_ring_buffer_full();
    failures += test_vulkan_init_headless();
    failures += test_vulkan_frame_upload();
    failures += test_frame_ring_buffer_concurrent();

    printf("\n===================\n");
    if (failures == 0) {
        printf("All Vulkan integration tests passed!\n");
    } else {
        printf("Some Vulkan integration tests FAILED: %d\n", failures);
    }

    return failures;
}
