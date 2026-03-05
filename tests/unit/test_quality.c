/*
 * test_quality.c — Unit tests for PHASE-39 Stream Quality Intelligence
 *
 * Tests quality_metrics (PSNR/SSIM/MSE), scene_detector, quality_monitor,
 * and quality_reporter using synthetic luma buffers.
 * No real video frames or hardware required.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/quality/quality_metrics.h"
#include "../../src/quality/scene_detector.h"
#include "../../src/quality/quality_monitor.h"
#include "../../src/quality/quality_reporter.h"

/* ── Test helpers ────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

#define W  64
#define H  48
#define ST 64  /* stride == width for test frames */

static uint8_t frame_a[H][ST];
static uint8_t frame_b[H][ST];

static void fill_solid(uint8_t buf[H][ST], uint8_t val) {
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            buf[y][x] = val;
}

static void fill_gradient(uint8_t buf[H][ST]) {
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            buf[y][x] = (uint8_t)((x + y) & 0xFF);
}

/* ── quality_metrics tests ───────────────────────────────────────── */

static int test_mse_identical(void) {
    printf("\n=== test_mse_identical ===\n");
    fill_gradient(frame_a);
    fill_gradient(frame_b);
    double mse = quality_mse((uint8_t *)frame_a, (uint8_t *)frame_b, W, H, ST);
    TEST_ASSERT(mse < 1e-10, "MSE of identical frames is ~0");
    TEST_PASS("MSE identical frames");
    return 0;
}

static int test_psnr_identical(void) {
    printf("\n=== test_psnr_identical ===\n");
    fill_gradient(frame_a);
    fill_gradient(frame_b);
    double psnr = quality_psnr((uint8_t *)frame_a, (uint8_t *)frame_b, W, H, ST);
    TEST_ASSERT(psnr >= 999.0, "PSNR of identical frames is sentinel 1000");
    TEST_PASS("PSNR identical frames → sentinel");
    return 0;
}

static int test_psnr_degraded(void) {
    printf("\n=== test_psnr_degraded ===\n");
    fill_gradient(frame_a);
    fill_gradient(frame_b);
    /* Add noise to frame_b */
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            frame_b[y][x] = (uint8_t)(frame_a[y][x] ^ 0x08); /* flip bit 3 */

    double psnr = quality_psnr((uint8_t *)frame_a, (uint8_t *)frame_b, W, H, ST);
    TEST_ASSERT(psnr > 10.0 && psnr < 60.0, "PSNR in plausible range");
    TEST_PASS("PSNR degraded frames in plausible range");
    return 0;
}

static int test_ssim_identical(void) {
    printf("\n=== test_ssim_identical ===\n");
    fill_gradient(frame_a);
    fill_gradient(frame_b);
    double ssim = quality_ssim((uint8_t *)frame_a, (uint8_t *)frame_b, W, H, ST);
    TEST_ASSERT(ssim > 0.999, "SSIM of identical frames ≈ 1.0");
    TEST_PASS("SSIM identical frames ≈ 1.0");
    return 0;
}

static int test_ssim_different(void) {
    printf("\n=== test_ssim_different ===\n");
    fill_solid(frame_a, 0);    /* black */
    fill_solid(frame_b, 255);  /* white */
    double ssim = quality_ssim((uint8_t *)frame_a, (uint8_t *)frame_b, W, H, ST);
    TEST_ASSERT(ssim < 0.5, "SSIM of black vs white is low");
    TEST_PASS("SSIM black vs white is low");
    return 0;
}

static int test_metrics_null_guards(void) {
    printf("\n=== test_metrics_null_guards ===\n");
    fill_gradient(frame_a);
    double v;
    v = quality_mse(NULL, (uint8_t *)frame_a, W, H, ST);
    TEST_ASSERT(v == 0.0, "MSE NULL ref returns 0");
    v = quality_psnr(NULL, (uint8_t *)frame_a, W, H, ST);
    TEST_ASSERT(v >= 999.0 || v == 0.0, "PSNR NULL ref safe");
    v = quality_ssim(NULL, (uint8_t *)frame_a, W, H, ST);
    TEST_ASSERT(v == 0.0, "SSIM NULL ref returns 0");
    TEST_PASS("metrics NULL pointer guards");
    return 0;
}

/* ── scene_detector tests ────────────────────────────────────────── */

static int test_scene_detector_create(void) {
    printf("\n=== test_scene_detector_create ===\n");
    scene_detector_t *det = scene_detector_create(NULL);
    TEST_ASSERT(det != NULL, "detector created with default config");
    TEST_ASSERT(scene_detector_frame_count(det) == 0, "initial frame count 0");
    scene_detector_destroy(det);
    scene_detector_destroy(NULL); /* must not crash */
    TEST_PASS("scene detector create/destroy");
    return 0;
}

static int test_scene_detector_no_change(void) {
    printf("\n=== test_scene_detector_no_change ===\n");
    scene_detector_t *det = scene_detector_create(NULL);
    TEST_ASSERT(det != NULL, "detector created");
    fill_gradient(frame_a);

    for (int i = 0; i < 5; i++) {
        scene_result_t r = scene_detector_push(det, (uint8_t *)frame_a,
                                                W, H, ST);
        if (i >= 2) {
            TEST_ASSERT(!r.scene_changed, "no scene change for identical frames");
            TEST_ASSERT(r.histogram_diff < 0.01, "histogram diff near 0");
        }
    }
    scene_detector_destroy(det);
    TEST_PASS("scene detector: no change on repeated identical frame");
    return 0;
}

static int test_scene_detector_cut(void) {
    printf("\n=== test_scene_detector_cut ===\n");
    scene_config_t cfg = { .threshold = 0.30, .warmup_frames = 1 };
    scene_detector_t *det = scene_detector_create(&cfg);
    TEST_ASSERT(det != NULL, "detector created");

    /* Push a few black frames */
    fill_solid(frame_a, 10);
    for (int i = 0; i < 3; i++) {
        scene_detector_push(det, (uint8_t *)frame_a, W, H, ST);
    }

    /* Push a white frame — should trigger cut */
    fill_solid(frame_b, 245);
    scene_result_t r = scene_detector_push(det, (uint8_t *)frame_b, W, H, ST);
    TEST_ASSERT(r.scene_changed, "scene change detected after black→white cut");
    TEST_ASSERT(r.histogram_diff > 0.30, "histogram diff > threshold");

    scene_detector_destroy(det);
    TEST_PASS("scene detector detects hard cut");
    return 0;
}

static int test_scene_detector_reset(void) {
    printf("\n=== test_scene_detector_reset ===\n");
    scene_detector_t *det = scene_detector_create(NULL);
    fill_gradient(frame_a);
    scene_detector_push(det, (uint8_t *)frame_a, W, H, ST);
    scene_detector_push(det, (uint8_t *)frame_a, W, H, ST);
    TEST_ASSERT(scene_detector_frame_count(det) == 2, "frame count 2");

    scene_detector_reset(det);
    TEST_ASSERT(scene_detector_frame_count(det) == 0, "frame count 0 after reset");
    scene_detector_destroy(det);
    TEST_PASS("scene detector reset");
    return 0;
}

/* ── quality_monitor tests ───────────────────────────────────────── */

static int test_monitor_create(void) {
    printf("\n=== test_monitor_create ===\n");
    quality_monitor_t *m = quality_monitor_create(NULL);
    TEST_ASSERT(m != NULL, "monitor created");
    TEST_ASSERT(!quality_monitor_is_degraded(m), "not degraded initially");
    quality_monitor_destroy(m);
    quality_monitor_destroy(NULL); /* must not crash */
    TEST_PASS("quality monitor create/destroy");
    return 0;
}

static int test_monitor_push_good(void) {
    printf("\n=== test_monitor_push_good ===\n");
    quality_monitor_config_t cfg = { 30.0, 0.85, 10 };
    quality_monitor_t *m = quality_monitor_create(&cfg);
    TEST_ASSERT(m != NULL, "monitor created");

    for (int i = 0; i < 10; i++) {
        quality_monitor_push(m, 40.0, 0.97);
    }
    TEST_ASSERT(!quality_monitor_is_degraded(m), "good quality: not degraded");

    quality_stats_t stats;
    quality_monitor_get_stats(m, &stats);
    TEST_ASSERT(stats.avg_psnr >= 39.0, "avg PSNR close to 40");
    TEST_ASSERT(stats.avg_ssim >= 0.96, "avg SSIM close to 0.97");
    TEST_ASSERT(stats.frames_total == 10, "frames_total == 10");

    quality_monitor_destroy(m);
    TEST_PASS("monitor with good quality: not degraded");
    return 0;
}

static int test_monitor_push_bad(void) {
    printf("\n=== test_monitor_push_bad ===\n");
    quality_monitor_config_t cfg = { 35.0, 0.90, 5 };
    quality_monitor_t *m = quality_monitor_create(&cfg);
    TEST_ASSERT(m != NULL, "monitor created");

    for (int i = 0; i < 5; i++) {
        quality_monitor_push(m, 20.0, 0.60); /* below threshold */
    }
    TEST_ASSERT(quality_monitor_is_degraded(m), "low quality: degraded");

    quality_stats_t stats;
    quality_monitor_get_stats(m, &stats);
    TEST_ASSERT(stats.alerts_total >= 1, "at least one alert fired");

    quality_monitor_destroy(m);
    TEST_PASS("monitor with bad quality: degraded + alert fired");
    return 0;
}

static int test_monitor_reset(void) {
    printf("\n=== test_monitor_reset ===\n");
    quality_monitor_config_t cfg = { 35.0, 0.90, 5 };
    quality_monitor_t *m = quality_monitor_create(&cfg);
    for (int i = 0; i < 5; i++) quality_monitor_push(m, 20.0, 0.50);
    TEST_ASSERT(quality_monitor_is_degraded(m), "degraded before reset");
    quality_monitor_reset(m);
    TEST_ASSERT(!quality_monitor_is_degraded(m), "not degraded after reset");
    quality_monitor_destroy(m);
    TEST_PASS("quality monitor reset");
    return 0;
}

/* ── quality_reporter tests ──────────────────────────────────────── */

static int test_reporter_basic(void) {
    printf("\n=== test_reporter_basic ===\n");
    quality_stats_t stats = {
        .avg_psnr     = 38.5,
        .avg_ssim     = 0.95,
        .min_psnr     = 31.2,
        .min_ssim     = 0.88,
        .frames_total = 600,
        .alerts_total = 2,
        .degraded     = false,
    };

    char buf[512];
    int n = quality_report_json(&stats, 7, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "report generated");
    TEST_ASSERT(strstr(buf, "\"frames_total\":600") != NULL,
                "frames_total in report");
    TEST_ASSERT(strstr(buf, "\"scene_changes\":7") != NULL,
                "scene_changes in report");
    TEST_ASSERT(strstr(buf, "\"degraded\":false") != NULL,
                "degraded:false in report");
    TEST_PASS("quality reporter basic JSON output");
    return 0;
}

static int test_reporter_buffer_too_small(void) {
    printf("\n=== test_reporter_buffer_too_small ===\n");
    quality_stats_t stats = { 0 };
    char buf[4];
    int n = quality_report_json(&stats, 0, buf, sizeof(buf));
    TEST_ASSERT(n == -1, "too-small buffer returns -1");
    TEST_PASS("reporter rejects too-small buffer");
    return 0;
}

static int test_reporter_null_guard(void) {
    printf("\n=== test_reporter_null_guard ===\n");
    char buf[256];
    int n = quality_report_json(NULL, 0, buf, sizeof(buf));
    TEST_ASSERT(n == -1, "NULL stats returns -1");
    TEST_PASS("reporter NULL stats guard");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_mse_identical();
    failures += test_psnr_identical();
    failures += test_psnr_degraded();
    failures += test_ssim_identical();
    failures += test_ssim_different();
    failures += test_metrics_null_guards();

    failures += test_scene_detector_create();
    failures += test_scene_detector_no_change();
    failures += test_scene_detector_cut();
    failures += test_scene_detector_reset();

    failures += test_monitor_create();
    failures += test_monitor_push_good();
    failures += test_monitor_push_bad();
    failures += test_monitor_reset();

    failures += test_reporter_basic();
    failures += test_reporter_buffer_too_small();
    failures += test_reporter_null_guard();

    printf("\n");
    if (failures == 0)
        printf("ALL QUALITY TESTS PASSED\n");
    else
        printf("%d QUALITY TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
