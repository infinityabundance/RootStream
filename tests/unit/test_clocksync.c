/*
 * test_clocksync.c — Unit tests for PHASE-65 Clock Sync estimator
 *
 * Tests cs_sample (init/rtt/offset), cs_filter (push/median/convergence),
 * and cs_stats (record/snapshot/avg/min/max/convergence/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "../../src/clocksync/cs_sample.h"
#include "../../src/clocksync/cs_filter.h"
#include "../../src/clocksync/cs_stats.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

/* ── cs_sample ───────────────────────────────────────────────────── */

static int test_sample_rtt_offset(void) {
    printf("\n=== test_sample_rtt_offset ===\n");

    cs_sample_t s;
    /* t0=1000, t3=3000 → elapsed=2000
     * t1=1500, t2=2500 → remote processing=1000
     * RTT = 2000 - 1000 = 1000µs
     * offset = ((1500-1000) + (2500-3000)) / 2 = (500 + -500) / 2 = 0 */
    TEST_ASSERT(cs_sample_init(&s, 1000, 1500, 2500, 3000) == 0, "init ok");
    TEST_ASSERT(cs_sample_rtt_us(&s) == 1000, "RTT = 1000µs");
    TEST_ASSERT(cs_sample_offset_us(&s) == 0,  "offset = 0");

    /* Remote clock 200µs ahead:
     * t0=0, t1=300, t2=500, t3=800
     * RTT = 800 - (500-300) = 800-200 = 600µs
     * offset = ((300-0) + (500-800)) / 2 = (300-300)/2 = 0 */
    cs_sample_init(&s, 0, 300, 500, 800);
    TEST_ASSERT(cs_sample_rtt_us(&s) == 600, "RTT 600µs");

    /* Offset of +200:
     * t0=0, t1=300, t2=300, t3=600
     * RTT = 600 - 0 = 600
     * offset = ((300-0) + (300-600))/2 = (300-300)/2 = 0 */
    cs_sample_init(&s, 0, 200, 400, 400);
    int64_t off = cs_sample_offset_us(&s);
    /* offset = ((200-0)+(400-400))/2 = 200/2 = 100µs */
    TEST_ASSERT(off == 100, "offset 100µs");

    TEST_ASSERT(cs_sample_init(NULL, 0,0,0,0) == -1, "NULL → -1");
    TEST_PASS("cs_sample rtt / offset");
    return 0;
}

/* ── cs_filter ───────────────────────────────────────────────────── */

static int test_filter_median(void) {
    printf("\n=== test_filter_median ===\n");

    cs_filter_t *f = cs_filter_create();
    TEST_ASSERT(f != NULL, "created");

    cs_filter_out_t out;

    /* Push 3 samples with offset 100, 200, 300 → median = 200 */
    cs_sample_t s;
    cs_sample_init(&s, 0, 100, 100, 0);  /* RTT=0, offset=100 */
    cs_filter_push(f, &s, &out);
    cs_sample_init(&s, 0, 200, 200, 0);  /* offset=200 */
    cs_filter_push(f, &s, &out);
    cs_sample_init(&s, 0, 300, 300, 0);  /* offset=300 */
    cs_filter_push(f, &s, &out);

    TEST_ASSERT(out.count == 3, "3 samples");
    TEST_ASSERT(!out.converged, "not yet converged (need 8)");
    TEST_ASSERT(out.offset_us == 200, "median offset = 200");

    /* Push 5 more identical samples to reach convergence */
    for (int i = 0; i < 5; i++) {
        cs_sample_init(&s, 0, 200, 200, 0);
        cs_filter_push(f, &s, &out);
    }
    TEST_ASSERT(out.converged, "converged after 8 samples");

    cs_filter_reset(f);
    cs_filter_push(f, &s, &out);
    TEST_ASSERT(out.count == 1, "reset clears count");

    cs_filter_destroy(f);
    TEST_PASS("cs_filter median / convergence / reset");
    return 0;
}

/* ── cs_stats ────────────────────────────────────────────────────── */

static int test_cs_stats(void) {
    printf("\n=== test_cs_stats ===\n");

    cs_stats_t *st = cs_stats_create();
    TEST_ASSERT(st != NULL, "created");

    cs_stats_record(st,  100, 1000);
    cs_stats_record(st, -200, 2000);
    cs_stats_record(st,  300, 3000);

    cs_stats_snapshot_t snap;
    int rc = cs_stats_snapshot(st, &snap);
    TEST_ASSERT(rc == 0, "snapshot ok");
    TEST_ASSERT(snap.sample_count == 3, "3 samples");
    TEST_ASSERT(snap.min_offset_us == -200, "min offset -200");
    TEST_ASSERT(snap.max_offset_us ==  300, "max offset 300");
    TEST_ASSERT(fabs(snap.avg_offset_us - 66.666) < 1.0, "avg offset ≈ 66.7");
    TEST_ASSERT(snap.min_rtt_us == 1000, "min RTT 1000");
    TEST_ASSERT(snap.max_rtt_us == 3000, "max RTT 3000");
    TEST_ASSERT(fabs(snap.avg_rtt_us - 2000.0) < 1.0, "avg RTT 2000");
    TEST_ASSERT(!snap.converged, "not converged (< 8 samples)");

    /* Feed 5 more to reach convergence */
    for (int i = 0; i < 5; i++) cs_stats_record(st, 0, 1000);
    cs_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.converged, "converged after 8 samples");

    cs_stats_reset(st);
    cs_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.sample_count == 0, "reset ok");

    cs_stats_destroy(st);
    TEST_PASS("cs_stats record/snapshot/min/max/avg/converged/reset");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_sample_rtt_offset();
    failures += test_filter_median();
    failures += test_cs_stats();

    printf("\n");
    if (failures == 0) printf("ALL CLOCKSYNC TESTS PASSED\n");
    else               printf("%d CLOCKSYNC TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
