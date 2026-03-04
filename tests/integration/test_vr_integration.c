#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../src/vr/openxr_manager.h"
#include "../../src/vr/vr_latency_optimizer.h"
#include "../../src/vr/head_tracker.h"
#include "../../src/vr/vr_profiler.h"

// Test helper macros (same pattern as tests/unit/test_vr.c)
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s\n", message); \
            return 1; \
        } \
    } while(0)

#define TEST_PASS(message) \
    do { \
        printf("PASS: %s\n", message); \
    } while(0)

// ── 24.9 integration tests ─────────────────────────────────────────────────

static int test_latency_optimizer_init(void) {
    printf("\n=== test_latency_optimizer_init ===\n");

    VRLatencyOptimizer *opt = vr_latency_optimizer_create();
    TEST_ASSERT(opt != NULL, "Latency optimizer creation");

    int result = vr_latency_optimizer_init(opt, VR_REPROJ_MOTION);
    TEST_ASSERT(result == 0, "Latency optimizer init with MOTION reprojection");

    vr_latency_optimizer_cleanup(opt);
    vr_latency_optimizer_destroy(opt);

    TEST_PASS("test_latency_optimizer_init");
    return 0;
}

static int test_latency_metrics_90hz(void) {
    printf("\n=== test_latency_metrics_90hz ===\n");

    VRLatencyOptimizer *opt = vr_latency_optimizer_create();
    TEST_ASSERT(opt != NULL, "Latency optimizer creation");
    TEST_ASSERT(vr_latency_optimizer_init(opt, VR_REPROJ_NONE) == 0, "Latency optimizer init");

    // 100 frames, each ~1 ms — well within 90 Hz budget
    struct timespec sleep_1ms = {0, 1000000L};  /* 1 ms */
    for (int i = 0; i < 100; i++) {
        vr_latency_optimizer_record_frame_start(opt);
        nanosleep(&sleep_1ms, NULL);
        vr_latency_optimizer_record_frame_end(opt);
    }

    VRLatencyMetrics m = vr_latency_optimizer_get_metrics(opt);
    TEST_ASSERT(m.meets_90hz_target == true, "90 Hz target met with 1 ms frames");

    vr_latency_optimizer_cleanup(opt);
    vr_latency_optimizer_destroy(opt);

    TEST_PASS("test_latency_metrics_90hz");
    return 0;
}

static int test_latency_metrics_slow(void) {
    printf("\n=== test_latency_metrics_slow ===\n");

    VRLatencyOptimizer *opt = vr_latency_optimizer_create();
    TEST_ASSERT(opt != NULL, "Latency optimizer creation");
    TEST_ASSERT(vr_latency_optimizer_init(opt, VR_REPROJ_NONE) == 0, "Latency optimizer init");

    // 20 frames, each ~15 ms — exceeds 11111 µs budget
    struct timespec sleep_15ms = {0, 15000000L}; /* 15 ms */
    for (int i = 0; i < 20; i++) {
        vr_latency_optimizer_record_frame_start(opt);
        nanosleep(&sleep_15ms, NULL);
        vr_latency_optimizer_record_frame_end(opt);
    }

    VRLatencyMetrics m = vr_latency_optimizer_get_metrics(opt);
    TEST_ASSERT(m.meets_90hz_target == false, "90 Hz target NOT met with 15 ms frames");

    vr_latency_optimizer_cleanup(opt);
    vr_latency_optimizer_destroy(opt);

    TEST_PASS("test_latency_metrics_slow");
    return 0;
}

static int test_reprojection_enabled(void) {
    printf("\n=== test_reprojection_enabled ===\n");

    VRLatencyOptimizer *opt = vr_latency_optimizer_create();
    TEST_ASSERT(opt != NULL, "Latency optimizer creation");
    TEST_ASSERT(vr_latency_optimizer_init(opt, VR_REPROJ_MOTION) == 0, "Latency optimizer init");

    // Call returns bool without crashing
    bool needed = vr_latency_optimizer_is_reprojection_needed(opt);
    (void)needed; /* either true or false is acceptable */

    vr_latency_optimizer_cleanup(opt);
    vr_latency_optimizer_destroy(opt);

    TEST_PASS("test_reprojection_enabled");
    return 0;
}

static int test_openxr_tracking_pipeline(void) {
    printf("\n=== test_openxr_tracking_pipeline ===\n");

    OpenXRManager *manager = openxr_manager_create();
    TEST_ASSERT(manager != NULL, "OpenXR manager creation");

    int result = openxr_manager_init(manager);
    TEST_ASSERT(result == 0, "OpenXR manager init");

    result = openxr_manager_create_session(manager);
    TEST_ASSERT(result == 0, "OpenXR session creation");

    XRState state = openxr_manager_get_tracking_data(manager);
    // Mock runtime returns zero-initialised poses; orientation.w == 1 by convention
    TEST_ASSERT(state.headOrientation.w == 1.0f, "Head orientation w == 1 (zero-init)");
    TEST_ASSERT(state.leftEyePose.position.x == 0.0f, "Left eye position zero-initialised");

    openxr_manager_cleanup(manager);
    openxr_manager_destroy(manager);

    TEST_PASS("test_openxr_tracking_pipeline");
    return 0;
}

static int test_profiler_vr_integration(void) {
    printf("\n=== test_profiler_vr_integration ===\n");

    VRProfiler *profiler = vr_profiler_create();
    TEST_ASSERT(profiler != NULL, "VR profiler creation");

    int result = vr_profiler_init(profiler);
    TEST_ASSERT(result == 0, "VR profiler init");

    VRFrameMetrics metrics = {
        .frametime_ms      = 11.1f,
        .rendertime_ms     = 8.5f,
        .fps               = 90.0f,
        .latency_ms        = 15.0f,
        .gpu_utilization   = 70.0f,
        .cpu_utilization   = 45.0f,
        .memory_usage_mb   = 1024.0f
    };
    result = vr_profiler_record_frame(profiler, &metrics);
    TEST_ASSERT(result == 0, "VR profiler record frame");

    VRFrameMetrics avg = vr_profiler_get_average_metrics(profiler, 1);
    TEST_ASSERT(avg.fps > 0.0f, "Average FPS > 0");

    vr_profiler_cleanup(profiler);
    vr_profiler_destroy(profiler);

    TEST_PASS("test_profiler_vr_integration");
    return 0;
}

// ── main ───────────────────────────────────────────────────────────────────

int main(void) {
    int failures = 0;

    printf("Starting VR Integration Tests...\n");

    failures += test_latency_optimizer_init();
    failures += test_latency_metrics_90hz();
    failures += test_latency_metrics_slow();
    failures += test_reprojection_enabled();
    failures += test_openxr_tracking_pipeline();
    failures += test_profiler_vr_integration();

    printf("\n===================\n");
    if (failures == 0) {
        printf("All VR integration tests passed!\n");
    } else {
        printf("VR integration test failures: %d\n", failures);
    }

    return failures;
}
