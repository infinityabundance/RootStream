/*
 * test_congestion.c — Unit tests for PHASE-56 Network Congestion Detector
 *
 * Tests rtt_estimator (first-sample init, SRTT/RTTVAR/RTO updates,
 * min/max tracking, reset), loss_detector (record/fraction/threshold/
 * congestion signal/reset/set_threshold), and congestion_stats
 * (integrated RTT+loss, event counting).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/congestion/rtt_estimator.h"
#include "../../src/congestion/loss_detector.h"
#include "../../src/congestion/congestion_stats.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

/* ── rtt_estimator tests ─────────────────────────────────────────── */

static int test_rtt_first_sample(void) {
    printf("\n=== test_rtt_first_sample ===\n");

    rtt_estimator_t *e = rtt_estimator_create();
    TEST_ASSERT(e != NULL, "created");
    TEST_ASSERT(!rtt_estimator_has_samples(e), "initially no samples");

    rtt_estimator_update(e, 20000); /* 20 ms */

    rtt_snapshot_t snap;
    rtt_estimator_snapshot(e, &snap);
    TEST_ASSERT(snap.sample_count == 1, "1 sample");
    /* First sample: SRTT = R = 20000, RTTVAR = R/2 = 10000 */
    TEST_ASSERT(fabs(snap.srtt_us   - 20000.0) < 1.0, "SRTT = R");
    TEST_ASSERT(fabs(snap.rttvar_us - 10000.0) < 1.0, "RTTVAR = R/2");
    /* RTO = SRTT + max(G, 4*RTTVAR) = 20000 + 40000 = 60000 */
    TEST_ASSERT(fabs(snap.rto_us - 60000.0) < 1.0, "RTO = 60 ms");
    TEST_ASSERT(fabs(snap.min_rtt_us - 20000.0) < 1.0, "min = first sample");

    rtt_estimator_destroy(e);
    TEST_PASS("rtt_estimator first-sample RFC6298 init");
    return 0;
}

static int test_rtt_convergence(void) {
    printf("\n=== test_rtt_convergence ===\n");

    rtt_estimator_t *e = rtt_estimator_create();

    /* Feed 100 identical 10ms samples; SRTT should converge to 10ms */
    for (int i = 0; i < 100; i++) rtt_estimator_update(e, 10000);

    rtt_snapshot_t snap;
    rtt_estimator_snapshot(e, &snap);
    TEST_ASSERT(fabs(snap.srtt_us - 10000.0) < 100.0, "SRTT converges to 10ms");
    TEST_ASSERT(snap.rttvar_us < 10.0, "RTTVAR near 0 for constant RTT");
    TEST_ASSERT(snap.min_rtt_us == 10000.0, "min = 10ms");
    TEST_ASSERT(snap.max_rtt_us == 10000.0, "max = 10ms");

    rtt_estimator_reset(e);
    TEST_ASSERT(!rtt_estimator_has_samples(e), "reset clears samples");

    rtt_estimator_destroy(e);
    TEST_PASS("rtt_estimator SRTT convergence");
    return 0;
}

static int test_rtt_null_guards(void) {
    printf("\n=== test_rtt_null_guards ===\n");

    TEST_ASSERT(rtt_estimator_update(NULL, 1000) == -1, "NULL estimator → -1");
    TEST_ASSERT(rtt_estimator_update(NULL, 0) == -1, "zero RTT → -1");

    rtt_snapshot_t snap;
    TEST_ASSERT(rtt_estimator_snapshot(NULL, &snap) == -1, "NULL snapshot → -1");

    TEST_PASS("rtt_estimator NULL guards");
    return 0;
}

/* ── loss_detector tests ─────────────────────────────────────────── */

static int test_loss_no_loss(void) {
    printf("\n=== test_loss_no_loss ===\n");

    loss_detector_t *d = loss_detector_create(0.05);
    TEST_ASSERT(d != NULL, "created");
    TEST_ASSERT(!loss_detector_is_congested(d), "initially not congested");

    for (int i = 0; i < 20; i++) {
        loss_signal_t s = loss_detector_record(d, LOSS_OUTCOME_RECEIVED);
        TEST_ASSERT(s == LOSS_SIGNAL_NONE, "no congestion");
    }
    TEST_ASSERT(fabs(loss_detector_loss_fraction(d)) < 0.001, "loss_fraction = 0");

    loss_detector_destroy(d);
    TEST_PASS("loss_detector no-loss path");
    return 0;
}

static int test_loss_congestion_trigger(void) {
    printf("\n=== test_loss_congestion_trigger ===\n");

    /* 10% threshold; send 10 good then 2 lost → >10% → congested */
    loss_detector_t *d = loss_detector_create(0.1);

    for (int i = 0; i < 10; i++) loss_detector_record(d, LOSS_OUTCOME_RECEIVED);
    loss_detector_record(d, LOSS_OUTCOME_LOST);
    loss_signal_t sig = loss_detector_record(d, LOSS_OUTCOME_LOST);
    TEST_ASSERT(sig == LOSS_SIGNAL_CONGESTED, "2/12 > 10% → CONGESTED");
    TEST_ASSERT(loss_detector_is_congested(d), "is_congested true");

    /* Reset → not congested */
    loss_detector_reset(d);
    TEST_ASSERT(!loss_detector_is_congested(d), "not congested after reset");
    TEST_ASSERT(fabs(loss_detector_loss_fraction(d)) < 0.001, "fraction = 0 after reset");

    loss_detector_destroy(d);
    TEST_PASS("loss_detector congestion trigger");
    return 0;
}

static int test_loss_set_threshold(void) {
    printf("\n=== test_loss_set_threshold ===\n");

    loss_detector_t *d = loss_detector_create(0.5);

    /* 50 good + 30 lost = 37.5% < 50%: not congested */
    for (int i = 0; i < 50; i++) loss_detector_record(d, LOSS_OUTCOME_RECEIVED);
    for (int i = 0; i < 30; i++) loss_detector_record(d, LOSS_OUTCOME_LOST);
    TEST_ASSERT(!loss_detector_is_congested(d), "below 50% threshold: not congested");

    /* Lower threshold to 0.2 */
    TEST_ASSERT(loss_detector_set_threshold(d, 0.2) == 0, "set_threshold ok");
    /* Re-record 1 packet to re-evaluate */
    loss_signal_t sig = loss_detector_record(d, LOSS_OUTCOME_LOST);
    TEST_ASSERT(sig == LOSS_SIGNAL_CONGESTED, "above 20% threshold after lowering");

    TEST_ASSERT(loss_detector_set_threshold(d, 2.0) == -1, "invalid threshold → -1");

    loss_detector_destroy(d);
    TEST_PASS("loss_detector set_threshold");
    return 0;
}

/* ── congestion_stats tests ──────────────────────────────────────── */

static int test_congestion_stats_integrated(void) {
    printf("\n=== test_congestion_stats_integrated ===\n");

    congestion_stats_t *cs = congestion_stats_create(0.1);
    TEST_ASSERT(cs != NULL, "created");

    /* Feed RTT samples */
    for (int i = 0; i < 5; i++) congestion_stats_record_rtt(cs, 15000);

    /* Good packets → not congested */
    for (int i = 0; i < 20; i++)
        congestion_stats_record_packet(cs, LOSS_OUTCOME_RECEIVED);

    /* Inject 5 consecutive losses → congestion onset */
    for (int i = 0; i < 5; i++)
        congestion_stats_record_packet(cs, LOSS_OUTCOME_LOST);

    /* Flush with good packets → recovery */
    for (int i = 0; i < 100; i++)
        congestion_stats_record_packet(cs, LOSS_OUTCOME_RECEIVED);

    congestion_snapshot_t snap;
    int rc = congestion_stats_snapshot(cs, &snap);
    TEST_ASSERT(rc == 0, "snapshot ok");
    TEST_ASSERT(snap.rtt.sample_count == 5, "5 RTT samples");
    TEST_ASSERT(fabs(snap.rtt.srtt_us - 15000.0) < 100.0, "SRTT ~15ms");
    TEST_ASSERT(snap.congestion_events >= 1, "at least 1 congestion event");
    TEST_ASSERT(snap.recovery_events   >= 1, "at least 1 recovery event");
    TEST_ASSERT(!snap.congested, "not congested at end (all good packets)");

    congestion_stats_reset(cs);
    congestion_stats_snapshot(cs, &snap);
    TEST_ASSERT(snap.rtt.sample_count == 0, "reset clears RTT samples");
    TEST_ASSERT(snap.congestion_events == 0, "reset clears events");

    congestion_stats_destroy(cs);
    TEST_PASS("congestion_stats integrated RTT+loss");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_rtt_first_sample();
    failures += test_rtt_convergence();
    failures += test_rtt_null_guards();

    failures += test_loss_no_loss();
    failures += test_loss_congestion_trigger();
    failures += test_loss_set_threshold();

    failures += test_congestion_stats_integrated();

    printf("\n");
    if (failures == 0)
        printf("ALL CONGESTION TESTS PASSED\n");
    else
        printf("%d CONGESTION TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
