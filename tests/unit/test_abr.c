/*
 * test_abr.c — Unit tests for PHASE-48 Adaptive Bitrate Controller
 *
 * Tests abr_estimator (EWMA update/reset/ready), abr_ladder (create/
 * select/sort), abr_controller (tick/downgrade/upgrade-hold/force), and
 * abr_stats (record/snapshot/reset).  No network hardware required.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/abr/abr_estimator.h"
#include "../../src/abr/abr_ladder.h"
#include "../../src/abr/abr_controller.h"
#include "../../src/abr/abr_stats.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── Test ladder helper ──────────────────────────────────────────── */

static abr_ladder_t *make_3level_ladder(void) {
    abr_level_t levels[3] = {
        {640,  360,  30, 500000,  30, "low"},
        {1280, 720,  30, 2000000, 60, "mid"},
        {1920, 1080, 30, 5000000, 85, "high"},
    };
    return abr_ladder_create(levels, 3);
}

/* ── abr_estimator tests ─────────────────────────────────────────── */

static int test_estimator_create(void) {
    printf("\n=== test_estimator_create ===\n");

    abr_estimator_t *e = abr_estimator_create(ABR_EWMA_ALPHA_DEFAULT);
    TEST_ASSERT(e != NULL, "estimator created");
    TEST_ASSERT(!abr_estimator_is_ready(e), "not ready with 0 samples");
    TEST_ASSERT(abr_estimator_get(e) == 0.0, "initial estimate 0");
    TEST_ASSERT(abr_estimator_sample_count(e) == 0, "initial count 0");

    abr_estimator_destroy(e);
    abr_estimator_destroy(NULL); /* must not crash */

    /* Invalid alpha */
    TEST_ASSERT(abr_estimator_create(0.0f) == NULL, "alpha=0 → NULL");
    TEST_ASSERT(abr_estimator_create(1.0f) == NULL, "alpha=1 → NULL");

    TEST_PASS("abr_estimator create/destroy");
    return 0;
}

static int test_estimator_ewma(void) {
    printf("\n=== test_estimator_ewma ===\n");

    abr_estimator_t *e = abr_estimator_create(0.5f);

    /* First sample: EWMA = sample */
    abr_estimator_update(e, 1000000.0);
    TEST_ASSERT(abr_estimator_get(e) == 1000000.0, "first sample initialises EWMA");

    /* Second sample: EWMA = 0.5*sample + 0.5*prev */
    abr_estimator_update(e, 3000000.0);
    double expected = 0.5 * 3000000.0 + 0.5 * 1000000.0;
    TEST_ASSERT(fabs(abr_estimator_get(e) - expected) < 1.0, "EWMA second sample");
    TEST_ASSERT(abr_estimator_sample_count(e) == 2, "count 2");

    TEST_PASS("abr_estimator EWMA");
    return 0;
}

static int test_estimator_ready(void) {
    printf("\n=== test_estimator_ready ===\n");

    abr_estimator_t *e = abr_estimator_create(ABR_EWMA_ALPHA_DEFAULT);
    for (int i = 0; i < ABR_ESTIMATOR_MIN_SAMPLES - 1; i++) {
        TEST_ASSERT(!abr_estimator_is_ready(e), "not ready before MIN_SAMPLES");
        abr_estimator_update(e, 2000000.0);
    }
    abr_estimator_update(e, 2000000.0);
    TEST_ASSERT(abr_estimator_is_ready(e), "ready at MIN_SAMPLES");

    abr_estimator_reset(e);
    TEST_ASSERT(!abr_estimator_is_ready(e), "not ready after reset");

    abr_estimator_destroy(e);
    TEST_PASS("abr_estimator ready/reset");
    return 0;
}

/* ── abr_ladder tests ────────────────────────────────────────────── */

static int test_ladder_create_count(void) {
    printf("\n=== test_ladder_create_count ===\n");

    abr_ladder_t *l = make_3level_ladder();
    TEST_ASSERT(l != NULL, "ladder created");
    TEST_ASSERT(abr_ladder_count(l) == 3, "3 levels");

    abr_level_t lv;
    int rc = abr_ladder_get(l, 0, &lv);
    TEST_ASSERT(rc == 0, "get level 0 ok");
    TEST_ASSERT(lv.bitrate_bps == 500000, "lowest level sorted first");

    rc = abr_ladder_get(l, 2, &lv);
    TEST_ASSERT(rc == 0, "get level 2 ok");
    TEST_ASSERT(lv.bitrate_bps == 5000000, "highest level last");

    TEST_ASSERT(abr_ladder_get(l, 3, &lv) == -1, "out-of-range → -1");

    abr_ladder_destroy(l);
    TEST_PASS("abr_ladder create/count/sort");
    return 0;
}

static int test_ladder_select(void) {
    printf("\n=== test_ladder_select ===\n");

    abr_ladder_t *l = make_3level_ladder();

    /* Budget exactly at low level */
    int idx = abr_ladder_select(l, 500000.0);
    TEST_ASSERT(idx == 0, "budget=500k → level 0");

    /* Budget between low and mid */
    idx = abr_ladder_select(l, 1000000.0);
    TEST_ASSERT(idx == 0, "budget=1M → level 0 (mid=2M doesn't fit)");

    /* Budget at mid level */
    idx = abr_ladder_select(l, 2000000.0);
    TEST_ASSERT(idx == 1, "budget=2M → level 1");

    /* Budget above all levels */
    idx = abr_ladder_select(l, 100000000.0);
    TEST_ASSERT(idx == 2, "budget=100M → level 2 (highest)");

    /* Budget below lowest */
    idx = abr_ladder_select(l, 100.0);
    TEST_ASSERT(idx == 0, "budget=100 → level 0 (minimum)");

    abr_ladder_destroy(l);
    TEST_PASS("abr_ladder select");
    return 0;
}

static int test_ladder_null_guards(void) {
    printf("\n=== test_ladder_null_guards ===\n");

    TEST_ASSERT(abr_ladder_create(NULL, 1) == NULL, "NULL levels");
    TEST_ASSERT(abr_ladder_create((const abr_level_t *)&(abr_level_t){0}, 0) == NULL,
                "0 levels");

    TEST_PASS("abr_ladder NULL guards");
    return 0;
}

/* ── abr_controller tests ────────────────────────────────────────── */

static int test_controller_create(void) {
    printf("\n=== test_controller_create ===\n");

    abr_estimator_t *e = abr_estimator_create(ABR_EWMA_ALPHA_DEFAULT);
    abr_ladder_t    *l = make_3level_ladder();
    abr_controller_t *c = abr_controller_create(e, l);
    TEST_ASSERT(c != NULL, "controller created");
    TEST_ASSERT(abr_controller_current_level(c) == 0, "starts at level 0");

    abr_controller_destroy(c);
    abr_estimator_destroy(e);
    abr_ladder_destroy(l);
    TEST_PASS("abr_controller create/destroy");
    return 0;
}

static int test_controller_downgrade(void) {
    printf("\n=== test_controller_downgrade ===\n");

    abr_estimator_t  *e = abr_estimator_create(0.5f);
    abr_ladder_t     *l = make_3level_ladder();
    abr_controller_t *c = abr_controller_create(e, l);

    /* Force to high level first */
    abr_controller_force_level(c, 2);
    TEST_ASSERT(abr_controller_current_level(c) == 2, "forced to level 2");

    /* Release force and feed low BW samples */
    abr_controller_force_level(c, -1);
    for (int i = 0; i < 5; i++)
        abr_estimator_update(e, 300000.0); /* below even low level */

    abr_decision_t d;
    abr_controller_tick(c, &d);
    TEST_ASSERT(d.new_level_idx < 2, "downgraded from level 2");
    TEST_ASSERT(d.is_downgrade, "is_downgrade set");

    abr_controller_destroy(c);
    abr_estimator_destroy(e);
    abr_ladder_destroy(l);
    TEST_PASS("abr_controller downgrade");
    return 0;
}

static int test_controller_upgrade_hold(void) {
    printf("\n=== test_controller_upgrade_hold ===\n");

    abr_estimator_t  *e = abr_estimator_create(0.9f);
    abr_ladder_t     *l = make_3level_ladder();
    abr_controller_t *c = abr_controller_create(e, l);

    /* Saturate estimator with high BW (above high-level bitrate) */
    for (int i = 0; i < 10; i++)
        abr_estimator_update(e, 10000000.0);

    /* First few ticks should stay at level 0 due to upgrade hold */
    abr_decision_t d;
    int prev_level = 0;
    for (int tick = 0; tick < ABR_UPGRADE_HOLD_TICKS; tick++) {
        abr_controller_tick(c, &d);
        if (tick < ABR_UPGRADE_HOLD_TICKS - 1) {
            /* Before hold period, should not have jumped to high */
            TEST_ASSERT(d.new_level_idx <= 1, "hold prevents rapid upgrade");
        }
        prev_level = d.new_level_idx;
    }
    (void)prev_level;

    /* Eventually should reach higher level */
    for (int tick = 0; tick < ABR_UPGRADE_HOLD_TICKS * 3; tick++)
        abr_controller_tick(c, &d);
    TEST_ASSERT(abr_controller_current_level(c) > 0, "eventually upgrades");

    abr_controller_destroy(c);
    abr_estimator_destroy(e);
    abr_ladder_destroy(l);
    TEST_PASS("abr_controller upgrade hold");
    return 0;
}

static int test_controller_force(void) {
    printf("\n=== test_controller_force ===\n");

    abr_estimator_t  *e = abr_estimator_create(ABR_EWMA_ALPHA_DEFAULT);
    abr_ladder_t     *l = make_3level_ladder();
    abr_controller_t *c = abr_controller_create(e, l);

    int rc = abr_controller_force_level(c, 2);
    TEST_ASSERT(rc == 0, "force level 2 ok");
    TEST_ASSERT(abr_controller_current_level(c) == 2, "at level 2");

    rc = abr_controller_force_level(c, 99);
    TEST_ASSERT(rc == -1, "force out-of-range returns -1");

    rc = abr_controller_force_level(c, -1);
    TEST_ASSERT(rc == 0, "release force ok");

    abr_controller_destroy(c);
    abr_estimator_destroy(e);
    abr_ladder_destroy(l);
    TEST_PASS("abr_controller force level");
    return 0;
}

/* ── abr_stats tests ─────────────────────────────────────────────── */

static int test_stats_record(void) {
    printf("\n=== test_stats_record ===\n");

    abr_stats_t *st = abr_stats_create();
    TEST_ASSERT(st != NULL, "stats created");

    abr_stats_record(st, 0, 0, 0);
    abr_stats_record(st, 1, 0, 0); /* upgrade */
    abr_stats_record(st, 0, 1, 0); /* downgrade */
    abr_stats_record(st, 0, 0, 1); /* stall */

    abr_stats_snapshot_t snap;
    int rc = abr_stats_snapshot(st, &snap);
    TEST_ASSERT(rc == 0, "snapshot ok");
    TEST_ASSERT(snap.total_ticks == 4, "4 ticks");
    TEST_ASSERT(snap.upgrade_count == 1, "1 upgrade");
    TEST_ASSERT(snap.downgrade_count == 1, "1 downgrade");
    TEST_ASSERT(snap.stall_ticks == 1, "1 stall");
    TEST_ASSERT(snap.ticks_per_level[0] == 3, "3 ticks at level 0");
    TEST_ASSERT(snap.ticks_per_level[1] == 1, "1 tick at level 1");

    abr_stats_reset(st);
    abr_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.total_ticks == 0, "reset clears ticks");

    abr_stats_destroy(st);
    TEST_PASS("abr_stats record/snapshot/reset");
    return 0;
}

static int test_stats_avg_level(void) {
    printf("\n=== test_stats_avg_level ===\n");

    abr_stats_t *st = abr_stats_create();
    /* 2 ticks at level 0, 2 at level 2 → avg = 1.0 */
    abr_stats_record(st, 0, 0, 0);
    abr_stats_record(st, 0, 0, 0);
    abr_stats_record(st, 2, 0, 0);
    abr_stats_record(st, 2, 2, 0);

    abr_stats_snapshot_t snap;
    abr_stats_snapshot(st, &snap);
    TEST_ASSERT(fabs(snap.avg_level - 1.0) < 0.01, "avg level = 1.0");

    abr_stats_destroy(st);
    TEST_PASS("abr_stats average level");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_estimator_create();
    failures += test_estimator_ewma();
    failures += test_estimator_ready();

    failures += test_ladder_create_count();
    failures += test_ladder_select();
    failures += test_ladder_null_guards();

    failures += test_controller_create();
    failures += test_controller_downgrade();
    failures += test_controller_upgrade_hold();
    failures += test_controller_force();

    failures += test_stats_record();
    failures += test_stats_avg_level();

    printf("\n");
    if (failures == 0)
        printf("ALL ABR TESTS PASSED\n");
    else
        printf("%d ABR TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
