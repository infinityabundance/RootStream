/*
 * test_timestamp.c — Unit tests for PHASE-71 Timestamp Synchronizer
 *
 * Tests ts_map (init/anchor/pts_to_us/us_to_pts),
 * ts_drift (init/update/ewma), and ts_stats
 * (record/max_drift/total_correction/snapshot/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "../../src/timestamp/ts_map.h"
#include "../../src/timestamp/ts_drift.h"
#include "../../src/timestamp/ts_stats.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── ts_map ──────────────────────────────────────────────────────── */

static int test_map_init(void) {
    printf("\n=== test_map_init ===\n");

    ts_map_t m;
    /* 90 kHz timebase: 1 tick = 1/90000 s = ~11.111 µs */
    TEST_ASSERT(ts_map_init(&m, 1, 90000) == 0, "init ok 90kHz");
    TEST_ASSERT(fabs(m.us_per_tick - (1e6 / 90000.0)) < 0.001, "us_per_tick 90kHz");

    TEST_ASSERT(ts_map_init(NULL, 1, 90000) == -1, "NULL → -1");
    TEST_ASSERT(ts_map_init(&m, 1, 0) == -1, "den=0 → -1");

    TEST_PASS("ts_map init / us_per_tick");
    return 0;
}

static int test_map_convert(void) {
    printf("\n=== test_map_convert ===\n");

    ts_map_t m;
    ts_map_init(&m, 1, 90000);

    /* Anchor at pts=0, wall=0 */
    ts_map_set_anchor(&m, 0, 0);

    /* 90000 ticks at 90kHz = 1 second = 1000000 µs */
    int64_t us = ts_map_pts_to_us(&m, 90000);
    TEST_ASSERT(llabs(us - 1000000) < 5, "90000 ticks → 1000000 µs");

    int64_t pts = ts_map_us_to_pts(&m, 1000000);
    TEST_ASSERT(llabs(pts - 90000) < 5, "1000000 µs → 90000 ticks");

    /* Uninitialised mapper returns 0 */
    ts_map_t m2;
    ts_map_init(&m2, 1, 90000);  /* not anchored */
    TEST_ASSERT(ts_map_pts_to_us(&m2, 1000) == 0, "uninit → 0");

    TEST_PASS("ts_map pts_to_us / us_to_pts / round-trip");
    return 0;
}

/* ── ts_drift ────────────────────────────────────────────────────── */

static int test_drift(void) {
    printf("\n=== test_drift ===\n");

    ts_drift_t d;
    TEST_ASSERT(ts_drift_init(&d) == 0, "init ok");
    TEST_ASSERT(ts_drift_init(NULL) == -1, "NULL → -1");

    /* First update: error = 500µs */
    int rc = ts_drift_update(&d, 1000500, 1000000);
    TEST_ASSERT(rc == 0, "update ok");
    TEST_ASSERT(d.sample_count == 1, "sample_count = 1");
    TEST_ASSERT(fabs(d.ewma_error_us - 500.0) < 0.1, "ewma init = 500");

    /* Second update: error = 0 → ewma decreases */
    ts_drift_update(&d, 2000000, 2000000);
    TEST_ASSERT(d.ewma_error_us < 500.0, "ewma decreases with 0 error");

    ts_drift_reset(&d);
    TEST_ASSERT(d.sample_count == 0, "reset ok");

    TEST_PASS("ts_drift update / ewma / reset");
    return 0;
}

/* ── ts_stats ────────────────────────────────────────────────────── */

static int test_ts_stats(void) {
    printf("\n=== test_ts_stats ===\n");

    ts_stats_t *st = ts_stats_create();
    TEST_ASSERT(st != NULL, "created");

    ts_stats_record(st,  300);
    ts_stats_record(st, -800);   /* abs = 800 → new max */
    ts_stats_record(st,  100);

    ts_stats_snapshot_t snap;
    TEST_ASSERT(ts_stats_snapshot(st, &snap) == 0, "snapshot ok");
    TEST_ASSERT(snap.sample_count == 3, "3 samples");
    TEST_ASSERT(snap.max_drift_us == 800, "max_drift = 800");
    TEST_ASSERT(snap.total_correction_us == 1200, "total = 1200");

    ts_stats_reset(st);
    ts_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.sample_count == 0, "reset ok");

    ts_stats_destroy(st);
    TEST_PASS("ts_stats record/max/total/snapshot/reset");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_map_init();
    failures += test_map_convert();
    failures += test_drift();
    failures += test_ts_stats();

    printf("\n");
    if (failures == 0) printf("ALL TIMESTAMP TESTS PASSED\n");
    else               printf("%d TIMESTAMP TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
