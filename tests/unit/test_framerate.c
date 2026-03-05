/*
 * test_framerate.c — Unit tests for PHASE-67 Frame Rate Controller
 *
 * Tests fr_limiter (init/tick/burst cap/set_fps/reset),
 * fr_target (init/mark/ewma/reset), and fr_stats
 * (record_frame/record_drop/snapshot/min/max/avg/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "../../src/framerate/fr_limiter.h"
#include "../../src/framerate/fr_target.h"
#include "../../src/framerate/fr_stats.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

/* ── fr_limiter ──────────────────────────────────────────────────── */

static int test_limiter_init(void) {
    printf("\n=== test_limiter_init ===\n");

    fr_limiter_t l;
    TEST_ASSERT(fr_limiter_init(&l, 30.0) == 0, "init ok");
    TEST_ASSERT(l.target_fps == 30.0, "target_fps");
    TEST_ASSERT(fr_limiter_init(NULL, 30.0) == -1, "NULL → -1");
    TEST_ASSERT(fr_limiter_init(&l,  0.0) == -1, "fps=0 → -1");
    TEST_ASSERT(fr_limiter_init(&l, -1.0) == -1, "fps<0 → -1");

    TEST_PASS("fr_limiter init");
    return 0;
}

static int test_limiter_tick(void) {
    printf("\n=== test_limiter_tick ===\n");

    fr_limiter_t l;
    fr_limiter_init(&l, 30.0);  /* 1 token per 33333µs */

    /* 0µs → 0 frames */
    TEST_ASSERT(fr_limiter_tick(&l, 0) == 0, "0µs → 0 frames");

    /* exactly 1 frame interval */
    int n = fr_limiter_tick(&l, 33334);
    TEST_ASSERT(n == 1, "1 frame interval → 1 frame");

    /* 2 full frame intervals */
    fr_limiter_reset(&l);
    n = fr_limiter_tick(&l, 66667);
    TEST_ASSERT(n == 2, "2 frame intervals → 2 frames");

    /* Burst cap: a very long gap should be capped at max_burst */
    fr_limiter_reset(&l);
    n = fr_limiter_tick(&l, 1000000); /* 1 second → 30 tokens, capped at 2 */
    TEST_ASSERT(n == FR_MAX_BURST, "burst cap = FR_MAX_BURST");

    /* set_fps at runtime */
    fr_limiter_reset(&l);
    TEST_ASSERT(fr_limiter_set_fps(&l, 60.0) == 0, "set_fps 60 ok");
    n = fr_limiter_tick(&l, 16667); /* 1 frame at 60fps */
    TEST_ASSERT(n == 1, "60fps: 1 frame/16667µs");

    TEST_PASS("fr_limiter tick / burst cap / set_fps");
    return 0;
}

/* ── fr_target ───────────────────────────────────────────────────── */

static int test_target_ewma(void) {
    printf("\n=== test_target_ewma ===\n");

    fr_target_t t;
    TEST_ASSERT(fr_target_init(&t, 30.0) == 0, "init ok");
    TEST_ASSERT(fabs(t.target_fps - 30.0) < 0.001, "target_fps");
    TEST_ASSERT(fr_target_init(NULL, 30.0) == -1, "NULL → -1");

    /* Mark frames 33333µs apart (30fps) */
    fr_target_mark(&t, 0);           /* first mark: sets base */
    fr_target_mark(&t, 33333);       /* interval = 33333 */
    fr_target_mark(&t, 66666);
    fr_target_mark(&t, 99999);

    TEST_ASSERT(t.frame_count == 4, "4 frames marked");
    /* actual_fps should be close to 30 */
    TEST_ASSERT(t.actual_fps > 25.0 && t.actual_fps < 35.0, "actual_fps ≈ 30");

    fr_target_reset(&t);
    TEST_ASSERT(t.frame_count == 0, "reset clears count");
    TEST_ASSERT(!t.initialised, "reset clears initialised");

    TEST_PASS("fr_target ewma / actual_fps / reset");
    return 0;
}

/* ── fr_stats ────────────────────────────────────────────────────── */

static int test_fr_stats(void) {
    printf("\n=== test_fr_stats ===\n");

    fr_stats_t *st = fr_stats_create();
    TEST_ASSERT(st != NULL, "created");

    fr_stats_record_frame(st, 30000);
    fr_stats_record_frame(st, 40000);
    fr_stats_record_frame(st, 20000);
    fr_stats_record_drop(st);
    fr_stats_record_drop(st);

    fr_stats_snapshot_t snap;
    TEST_ASSERT(fr_stats_snapshot(st, &snap) == 0, "snapshot ok");
    TEST_ASSERT(snap.frame_count == 3, "3 frames");
    TEST_ASSERT(snap.drop_count  == 2, "2 drops");
    TEST_ASSERT(snap.min_interval_us == 20000, "min 20000");
    TEST_ASSERT(snap.max_interval_us == 40000, "max 40000");
    TEST_ASSERT(fabs(snap.avg_interval_us - 30000.0) < 1.0, "avg 30000");

    fr_stats_reset(st);
    fr_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.frame_count == 0, "reset ok");

    fr_stats_destroy(st);
    TEST_PASS("fr_stats record/snapshot/min/max/avg/drop/reset");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_limiter_init();
    failures += test_limiter_tick();
    failures += test_target_ewma();
    failures += test_fr_stats();

    printf("\n");
    if (failures == 0) printf("ALL FRAMERATE TESTS PASSED\n");
    else               printf("%d FRAMERATE TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
