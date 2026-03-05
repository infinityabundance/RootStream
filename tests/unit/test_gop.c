/*
 * test_gop.c — Unit tests for PHASE-62 Adaptive GOP Controller
 *
 * Tests gop_policy (default/validate), gop_controller (natural IDR,
 * scene-change IDR, loss-recovery IDR, high-RTT suppression, cooldown,
 * force_idr, update_policy, decision/reason names), and gop_stats
 * (record natural/scene/loss, snapshot, avg GOP length, reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/gop/gop_policy.h"
#include "../../src/gop/gop_controller.h"
#include "../../src/gop/gop_stats.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

/* ── gop_policy tests ────────────────────────────────────────────── */

static int test_policy_default_validate(void) {
    printf("\n=== test_policy_default_validate ===\n");

    gop_policy_t p;
    TEST_ASSERT(gop_policy_default(&p) == 0, "default ok");
    TEST_ASSERT(gop_policy_validate(&p) == 0, "default valid");
    TEST_ASSERT(p.min_gop_frames == GOP_DEFAULT_MIN_FRAMES, "min_gop");
    TEST_ASSERT(p.max_gop_frames == GOP_DEFAULT_MAX_FRAMES, "max_gop");

    /* Invalid: min > max */
    p.min_gop_frames = 100; p.max_gop_frames = 50;
    TEST_ASSERT(gop_policy_validate(&p) == -1, "min > max → invalid");

    /* Invalid: scene threshold out of range */
    gop_policy_default(&p);
    p.scene_change_threshold = 1.5f;
    TEST_ASSERT(gop_policy_validate(&p) == -1, "threshold > 1 → invalid");

    TEST_ASSERT(gop_policy_default(NULL) == -1, "NULL → -1");
    TEST_ASSERT(gop_policy_validate(NULL) == -1, "NULL → -1");

    TEST_PASS("gop_policy default / validate");
    return 0;
}

/* ── gop_controller tests ────────────────────────────────────────── */

static gop_controller_t *make_gc(int min_frames, int max_frames,
                                  float scene_thr, float loss_thr,
                                  uint64_t rtt_thr) {
    gop_policy_t p;
    gop_policy_default(&p);
    p.min_gop_frames         = min_frames;
    p.max_gop_frames         = max_frames;
    p.scene_change_threshold = scene_thr;
    p.loss_threshold         = loss_thr;
    p.rtt_threshold_us       = rtt_thr;
    return gop_controller_create(&p);
}

static int test_gc_natural_idr(void) {
    printf("\n=== test_gc_natural_idr ===\n");

    gop_controller_t *gc = make_gc(2, 10, 0.9f, 0.05f, 200000);
    TEST_ASSERT(gc != NULL, "created");

    gop_reason_t reason;
    int idr_count = 0;
    for (int f = 0; f < 12; f++) {
        gop_decision_t d = gop_controller_next_frame(gc, 0.0f, 10000, 0.0f, &reason);
        if (d == GOP_DECISION_IDR) idr_count++;
    }
    /* With max=10, expect exactly 1 natural IDR in 12 frames */
    TEST_ASSERT(idr_count >= 1, "at least 1 natural IDR");
    TEST_ASSERT(reason == GOP_REASON_NATURAL || reason == GOP_REASON_NONE,
                "reason NATURAL or NONE after burst");

    gop_controller_destroy(gc);
    TEST_PASS("gop_controller natural IDR at max_gop");
    return 0;
}

static int test_gc_scene_change(void) {
    printf("\n=== test_gc_scene_change ===\n");

    /* min=2, max=300, scene_thr=0.5 */
    gop_controller_t *gc = make_gc(2, 300, 0.5f, 0.05f, 200000);

    /* Advance past cooldown */
    for (int f = 0; f < 3; f++)
        gop_controller_next_frame(gc, 0.0f, 10000, 0.0f, NULL);

    /* Now inject a scene-change */
    gop_reason_t reason;
    gop_decision_t d = gop_controller_next_frame(gc, 0.9f, 10000, 0.0f, &reason);
    TEST_ASSERT(d == GOP_DECISION_IDR, "scene change → IDR");
    TEST_ASSERT(reason == GOP_REASON_SCENE_CHANGE, "reason = SCENE_CHANGE");
    TEST_ASSERT(gop_controller_frames_since_idr(gc) == 0, "counter reset");

    gop_controller_destroy(gc);
    TEST_PASS("gop_controller scene-change IDR");
    return 0;
}

static int test_gc_loss_recovery(void) {
    printf("\n=== test_gc_loss_recovery ===\n");

    /* min=2, max=300, loss_thr=0.03, rtt_thr=200ms */
    gop_controller_t *gc = make_gc(2, 300, 0.9f, 0.03f, 200000);

    /* Advance past cooldown */
    for (int f = 0; f < 3; f++)
        gop_controller_next_frame(gc, 0.0f, 10000, 0.0f, NULL);

    gop_reason_t reason;
    /* High loss, low RTT → IDR */
    gop_decision_t d = gop_controller_next_frame(gc, 0.0f, 10000, 0.1f, &reason);
    TEST_ASSERT(d == GOP_DECISION_IDR, "loss > threshold → IDR");
    TEST_ASSERT(reason == GOP_REASON_LOSS_RECOVERY, "reason = LOSS_RECOVERY");

    gop_controller_destroy(gc);
    TEST_PASS("gop_controller loss-recovery IDR");
    return 0;
}

static int test_gc_high_rtt_suppression(void) {
    printf("\n=== test_gc_high_rtt_suppression ===\n");

    gop_controller_t *gc = make_gc(2, 300, 0.9f, 0.03f, 200000);

    for (int f = 0; f < 3; f++)
        gop_controller_next_frame(gc, 0.0f, 10000, 0.0f, NULL);

    /* High loss BUT also high RTT (> rtt_threshold) → no loss IDR */
    gop_reason_t reason;
    gop_decision_t d = gop_controller_next_frame(gc, 0.0f, 300000, 0.1f, &reason);
    TEST_ASSERT(d == GOP_DECISION_P_FRAME, "high RTT suppresses loss IDR");
    TEST_ASSERT(reason == GOP_REASON_NONE, "reason = NONE");

    gop_controller_destroy(gc);
    TEST_PASS("gop_controller high-RTT loss suppression");
    return 0;
}

static int test_gc_cooldown(void) {
    printf("\n=== test_gc_cooldown ===\n");

    /* min=5 cooldown */
    gop_controller_t *gc = make_gc(5, 300, 0.5f, 0.03f, 200000);

    /* Frame 1: past cooldown? No (frames_since_idr=1 ≤ min=5) */
    gop_reason_t reason;
    gop_decision_t d = gop_controller_next_frame(gc, 0.99f, 10000, 0.99f, &reason);
    TEST_ASSERT(d == GOP_DECISION_P_FRAME, "within cooldown: no IDR despite scene+loss");

    gop_controller_destroy(gc);
    TEST_PASS("gop_controller cooldown respected");
    return 0;
}

static int test_gc_force_idr(void) {
    printf("\n=== test_gc_force_idr ===\n");

    gop_controller_t *gc = make_gc(2, 300, 0.9f, 0.03f, 200000);
    for (int f = 0; f < 10; f++)
        gop_controller_next_frame(gc, 0.0f, 10000, 0.0f, NULL);

    gop_controller_force_idr(gc);
    TEST_ASSERT(gop_controller_frames_since_idr(gc) == 0, "force_idr resets counter");

    gop_controller_destroy(gc);
    TEST_PASS("gop_controller force_idr");
    return 0;
}

static int test_gc_names(void) {
    printf("\n=== test_gc_names ===\n");

    TEST_ASSERT(strcmp(gop_decision_name(GOP_DECISION_P_FRAME), "P_FRAME") == 0, "P_FRAME");
    TEST_ASSERT(strcmp(gop_decision_name(GOP_DECISION_IDR),     "IDR")     == 0, "IDR");
    TEST_ASSERT(strcmp(gop_reason_name(GOP_REASON_NATURAL),       "NATURAL")       == 0, "NATURAL");
    TEST_ASSERT(strcmp(gop_reason_name(GOP_REASON_SCENE_CHANGE),  "SCENE_CHANGE")  == 0, "SCENE_CHANGE");
    TEST_ASSERT(strcmp(gop_reason_name(GOP_REASON_LOSS_RECOVERY), "LOSS_RECOVERY") == 0, "LOSS_RECOVERY");
    TEST_ASSERT(strcmp(gop_reason_name(GOP_REASON_NONE),          "NONE")          == 0, "NONE");

    TEST_PASS("gop decision/reason names");
    return 0;
}

/* ── gop_stats tests ─────────────────────────────────────────────── */

static int test_gop_stats(void) {
    printf("\n=== test_gop_stats ===\n");

    gop_stats_t *st = gop_stats_create();
    TEST_ASSERT(st != NULL, "created");

    /* Simulate: 30 P-frames then natural IDR, 15 P-frames then scene IDR */
    for (int f = 0; f < 30; f++) gop_stats_record(st, 0, GOP_REASON_NONE);
    gop_stats_record(st, 1, GOP_REASON_NATURAL);
    for (int f = 0; f < 15; f++) gop_stats_record(st, 0, GOP_REASON_NONE);
    gop_stats_record(st, 1, GOP_REASON_SCENE_CHANGE);
    gop_stats_record(st, 1, GOP_REASON_LOSS_RECOVERY);

    gop_stats_snapshot_t snap;
    int rc = gop_stats_snapshot(st, &snap);
    TEST_ASSERT(rc == 0, "snapshot ok");
    TEST_ASSERT(snap.total_frames == 33 + 15, "total frames");
    TEST_ASSERT(snap.idr_natural == 1, "1 natural IDR");
    TEST_ASSERT(snap.idr_scene_change == 1, "1 scene IDR");
    TEST_ASSERT(snap.idr_loss_recovery == 1, "1 loss IDR");
    TEST_ASSERT(snap.total_idrs == 3, "3 total IDRs");
    /* avg GOP = (31 + 16 + 1) / 3 = 16.0 */
    TEST_ASSERT(fabs(snap.avg_gop_length - 16.0) < 1.0, "avg GOP ≈ 16");

    gop_stats_reset(st);
    gop_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.total_frames == 0, "reset ok");

    gop_stats_destroy(st);
    TEST_PASS("gop_stats natural/scene/loss/snapshot/avg/reset");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_policy_default_validate();
    failures += test_gc_natural_idr();
    failures += test_gc_scene_change();
    failures += test_gc_loss_recovery();
    failures += test_gc_high_rtt_suppression();
    failures += test_gc_cooldown();
    failures += test_gc_force_idr();
    failures += test_gc_names();
    failures += test_gop_stats();

    printf("\n");
    if (failures == 0)
        printf("ALL GOP TESTS PASSED\n");
    else
        printf("%d GOP TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
