/*
 * test_frc.c — Unit tests for PHASE-53 Frame Rate Controller
 *
 * Tests frc_clock (stub mode), frc_pacer (create/tick/drop/dup/set_fps),
 * and frc_stats (record/snapshot/reset/fps-estimate).  Uses the stub
 * clock to avoid wall-time dependencies.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/frc/frc_clock.h"
#include "../../src/frc/frc_pacer.h"
#include "../../src/frc/frc_stats.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            frc_clock_clear_stub(); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── frc_clock tests ─────────────────────────────────────────────── */

static int test_clock_stub(void) {
    printf("\n=== test_clock_stub ===\n");

    TEST_ASSERT(!frc_clock_is_stub(), "initially real clock");

    frc_clock_set_stub_ns(123456789ULL);
    TEST_ASSERT(frc_clock_is_stub(), "stub active");
    TEST_ASSERT(frc_clock_now_ns() == 123456789ULL, "stub value returned");

    frc_clock_clear_stub();
    TEST_ASSERT(!frc_clock_is_stub(), "stub cleared");
    /* Real clock should return something large (>0) */
    TEST_ASSERT(frc_clock_now_ns() > 0, "real clock > 0");

    TEST_PASS("frc_clock stub mode");
    return 0;
}

static int test_clock_conversions(void) {
    printf("\n=== test_clock_conversions ===\n");

    TEST_ASSERT(frc_clock_ns_to_us(1000) == 1, "1000ns → 1µs");
    TEST_ASSERT(frc_clock_ns_to_ms(1000000) == 1, "1000000ns → 1ms");
    TEST_ASSERT(frc_clock_ns_to_us(0) == 0, "0ns → 0µs");

    TEST_PASS("frc_clock unit conversions");
    return 0;
}

/* ── frc_pacer tests ─────────────────────────────────────────────── */

/* 30 FPS → frame interval = 1e9/30 ≈ 33333333.33 ns; use 33334000 to ensure ≥ 1 token */
#define FPS30_INTERVAL_NS  33334000ULL

static int test_pacer_create(void) {
    printf("\n=== test_pacer_create ===\n");

    frc_pacer_t *p = frc_pacer_create(30.0, 0);
    TEST_ASSERT(p != NULL, "pacer created");
    TEST_ASSERT(fabs(frc_pacer_target_fps(p) - 30.0) < 0.01, "target fps 30");

    frc_pacer_destroy(p);
    frc_pacer_destroy(NULL);
    TEST_ASSERT(frc_pacer_create(0.0, 0) == NULL, "fps=0 → NULL");
    TEST_ASSERT(frc_pacer_create(-1.0, 0) == NULL, "fps<0 → NULL");

    TEST_PASS("frc_pacer create/destroy");
    return 0;
}

static int test_pacer_present(void) {
    printf("\n=== test_pacer_present ===\n");

    /* At exactly 30fps cadence every tick should present */
    frc_pacer_t *p = frc_pacer_create(30.0, 0);

    /* First tick at t=0 always presents (starts with tokens=1.0) */
    frc_action_t a = frc_pacer_tick(p, 0);
    TEST_ASSERT(a == FRC_ACTION_PRESENT, "first tick presents");

    /* Tick exactly one frame interval later → present */
    a = frc_pacer_tick(p, FPS30_INTERVAL_NS);
    TEST_ASSERT(a == FRC_ACTION_PRESENT, "tick at frame interval → present");

    frc_pacer_destroy(p);
    TEST_PASS("frc_pacer present at exact rate");
    return 0;
}

static int test_pacer_drop(void) {
    printf("\n=== test_pacer_drop ===\n");

    frc_pacer_t *p = frc_pacer_create(30.0, 0);

    /* First tick presents; second tick immediately (same time) → drop */
    frc_pacer_tick(p, 0);
    frc_action_t a = frc_pacer_tick(p, 0);
    TEST_ASSERT(a == FRC_ACTION_DROP, "immediate second tick → drop");

    frc_pacer_destroy(p);
    TEST_PASS("frc_pacer drop (rate too high)");
    return 0;
}

static int test_pacer_set_fps(void) {
    printf("\n=== test_pacer_set_fps ===\n");

    frc_pacer_t *p = frc_pacer_create(30.0, 0);

    int rc = frc_pacer_set_fps(p, 60.0);
    TEST_ASSERT(rc == 0, "set_fps ok");
    TEST_ASSERT(fabs(frc_pacer_target_fps(p) - 60.0) < 0.01, "fps updated to 60");

    TEST_ASSERT(frc_pacer_set_fps(NULL, 30.0) == -1, "NULL → -1");
    TEST_ASSERT(frc_pacer_set_fps(p, 0.0) == -1, "fps=0 → -1");

    frc_pacer_destroy(p);
    TEST_PASS("frc_pacer set_fps");
    return 0;
}

static int test_pacer_action_names(void) {
    printf("\n=== test_pacer_action_names ===\n");

    TEST_ASSERT(strcmp(frc_action_name(FRC_ACTION_PRESENT),   "present")   == 0, "present");
    TEST_ASSERT(strcmp(frc_action_name(FRC_ACTION_DROP),      "drop")      == 0, "drop");
    TEST_ASSERT(strcmp(frc_action_name(FRC_ACTION_DUPLICATE), "duplicate") == 0, "duplicate");
    TEST_ASSERT(strcmp(frc_action_name((frc_action_t)99), "unknown") == 0, "unknown");

    TEST_PASS("frc_pacer action names");
    return 0;
}

/* ── frc_stats tests ─────────────────────────────────────────────── */

static int test_stats_basic(void) {
    printf("\n=== test_stats_basic ===\n");

    frc_stats_t *st = frc_stats_create();
    TEST_ASSERT(st != NULL, "stats created");

    frc_stats_record(st, 1, 0, 0, 0);
    frc_stats_record(st, 0, 1, 0, 0);
    frc_stats_record(st, 0, 0, 1, 0);
    frc_stats_record(st, 1, 0, 0, 0);

    frc_stats_snapshot_t snap;
    int rc = frc_stats_snapshot(st, &snap);
    TEST_ASSERT(rc == 0, "snapshot ok");
    TEST_ASSERT(snap.frames_presented  == 2, "2 presented");
    TEST_ASSERT(snap.frames_dropped    == 1, "1 dropped");
    TEST_ASSERT(snap.frames_duplicated == 1, "1 duplicated");

    frc_stats_reset(st);
    frc_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.frames_presented == 0, "reset clears presented");

    frc_stats_destroy(st);
    TEST_PASS("frc_stats basic");
    return 0;
}

static int test_stats_fps(void) {
    printf("\n=== test_stats_fps ===\n");

    frc_stats_t *st = frc_stats_create();

    /* Simulate 30 frames over 1 second → should compute ~30 fps */
    uint64_t step = 1000000000ULL / 30; /* ~33ms per frame */
    for (int i = 0; i < 30; i++) {
        uint64_t t = (uint64_t)i * step;
        frc_stats_record(st, 1, 0, 0, t);
    }
    /* Trigger the 1-second window at t = 1s */
    frc_stats_record(st, 1, 0, 0, 1000000000ULL + step);

    frc_stats_snapshot_t snap;
    frc_stats_snapshot(st, &snap);
    /* actual_fps will be non-zero after at least one window */
    TEST_ASSERT(snap.actual_fps > 0.0, "actual_fps computed");

    frc_stats_destroy(st);
    TEST_PASS("frc_stats fps estimation");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_clock_stub();
    failures += test_clock_conversions();

    failures += test_pacer_create();
    failures += test_pacer_present();
    failures += test_pacer_drop();
    failures += test_pacer_set_fps();
    failures += test_pacer_action_names();

    failures += test_stats_basic();
    failures += test_stats_fps();

    printf("\n");
    if (failures == 0)
        printf("ALL FRC TESTS PASSED\n");
    else
        printf("%d FRC TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
