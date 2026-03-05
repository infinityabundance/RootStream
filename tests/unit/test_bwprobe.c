/*
 * test_bwprobe.c — Unit tests for PHASE-60 Bandwidth Probe
 *
 * Tests probe_packet (encode/decode/bad-magic), probe_scheduler
 * (first-tick send, burst completion, interval enforcement,
 * burst/packet counts, set_interval), and probe_estimator
 * (OWD first-sample/EWMA/min-max, bandwidth estimation, reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/bwprobe/probe_packet.h"
#include "../../src/bwprobe/probe_scheduler.h"
#include "../../src/bwprobe/probe_estimator.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

/* ── probe_packet tests ──────────────────────────────────────────── */

static int test_probe_pkt_roundtrip(void) {
    printf("\n=== test_probe_pkt_roundtrip ===\n");

    probe_packet_t pkt = {
        .seq        = 7,
        .size_hint  = PROBE_PKT_SIZE,
        .send_ts_us = 123456789ULL,
        .burst_id   = 3,
        .burst_seq  = 1,
    };
    uint8_t buf[PROBE_PKT_SIZE];
    int n = probe_packet_encode(&pkt, buf, sizeof(buf));
    TEST_ASSERT(n == PROBE_PKT_SIZE, "encoded size");

    probe_packet_t dec;
    int rc = probe_packet_decode(buf, sizeof(buf), &dec);
    TEST_ASSERT(rc == 0, "decode ok");
    TEST_ASSERT(dec.seq        == 7,            "seq");
    TEST_ASSERT(dec.size_hint  == PROBE_PKT_SIZE,"size_hint");
    TEST_ASSERT(dec.send_ts_us == 123456789ULL, "send_ts_us");
    TEST_ASSERT(dec.burst_id   == 3,            "burst_id");
    TEST_ASSERT(dec.burst_seq  == 1,            "burst_seq");

    TEST_PASS("probe_packet round-trip");
    return 0;
}

static int test_probe_pkt_bad_magic(void) {
    printf("\n=== test_probe_pkt_bad_magic ===\n");

    uint8_t buf[PROBE_PKT_SIZE] = {0};
    probe_packet_t dec;
    TEST_ASSERT(probe_packet_decode(buf, sizeof(buf), &dec) == -1, "bad magic → -1");

    TEST_PASS("probe_packet bad magic rejected");
    return 0;
}

/* ── probe_scheduler tests ───────────────────────────────────────── */

static int test_sched_first_tick(void) {
    printf("\n=== test_sched_first_tick ===\n");

    probe_scheduler_t *s = probe_scheduler_create(200000, 3);
    TEST_ASSERT(s != NULL, "created");

    probe_packet_t pkt;
    /* First tick always sends */
    probe_sched_decision_t d = probe_scheduler_tick(s, 0, &pkt);
    TEST_ASSERT(d == PROBE_SCHED_SEND, "first tick SEND");
    TEST_ASSERT(pkt.burst_seq == 0, "burst_seq = 0");
    TEST_ASSERT(probe_scheduler_burst_count(s) == 1, "1 burst");
    TEST_ASSERT(probe_scheduler_packet_count(s) == 1, "1 packet");

    probe_scheduler_destroy(s);
    TEST_PASS("probe_scheduler first tick always sends");
    return 0;
}

static int test_sched_burst_completion(void) {
    printf("\n=== test_sched_burst_completion ===\n");

    probe_scheduler_t *s = probe_scheduler_create(200000, 3);
    probe_packet_t pkt;

    /* Burst of 3: seq 0, 1, 2 */
    probe_sched_decision_t d0 = probe_scheduler_tick(s, 0, &pkt);
    TEST_ASSERT(d0 == PROBE_SCHED_SEND && pkt.burst_seq == 0, "seq 0 sent");

    probe_sched_decision_t d1 = probe_scheduler_tick(s, 0, &pkt);
    TEST_ASSERT(d1 == PROBE_SCHED_SEND && pkt.burst_seq == 1, "seq 1 sent");

    probe_sched_decision_t d2 = probe_scheduler_tick(s, 0, &pkt);
    TEST_ASSERT(d2 == PROBE_SCHED_SEND && pkt.burst_seq == 2, "seq 2 sent");

    /* After burst complete, next tick before interval → WAIT */
    probe_sched_decision_t d3 = probe_scheduler_tick(s, 1000, &pkt);
    TEST_ASSERT(d3 == PROBE_SCHED_WAIT, "within interval → WAIT");

    TEST_ASSERT(probe_scheduler_packet_count(s) == 3, "3 packets total");
    TEST_ASSERT(probe_scheduler_burst_count(s) == 1, "1 burst");

    /* After interval elapses → sends again */
    probe_sched_decision_t d4 = probe_scheduler_tick(s, 200001, &pkt);
    TEST_ASSERT(d4 == PROBE_SCHED_SEND, "after interval → SEND");
    TEST_ASSERT(probe_scheduler_burst_count(s) == 2, "2 bursts");

    probe_scheduler_destroy(s);
    TEST_PASS("probe_scheduler burst completion + interval");
    return 0;
}

static int test_sched_set_interval(void) {
    printf("\n=== test_sched_set_interval ===\n");

    probe_scheduler_t *s = probe_scheduler_create(200000, 1);
    probe_packet_t pkt;
    probe_scheduler_tick(s, 0, &pkt); /* kick first burst */

    TEST_ASSERT(probe_scheduler_set_interval(s, 50000) == 0, "set_interval ok");
    TEST_ASSERT(probe_scheduler_set_interval(s, 0) == -1, "zero interval → -1");

    /* Now interval is 50ms; tick at 50001µs → send */
    probe_sched_decision_t d = probe_scheduler_tick(s, 50001, &pkt);
    TEST_ASSERT(d == PROBE_SCHED_SEND, "new interval respected");

    probe_scheduler_destroy(s);
    TEST_PASS("probe_scheduler set_interval");
    return 0;
}

/* ── probe_estimator tests ───────────────────────────────────────── */

static int test_estimator_owd(void) {
    printf("\n=== test_estimator_owd ===\n");

    probe_estimator_t *pe = probe_estimator_create();
    TEST_ASSERT(pe != NULL, "created");
    TEST_ASSERT(!probe_estimator_has_samples(pe), "initially no samples");

    /* OWD = 10000µs */
    probe_estimator_observe(pe, 0, 10000, 32);
    TEST_ASSERT(probe_estimator_has_samples(pe), "has samples");

    probe_estimate_t est;
    probe_estimator_snapshot(pe, &est);
    TEST_ASSERT(fabs(est.owd_us - 10000.0) < 1.0, "OWD = 10ms on first sample");
    TEST_ASSERT(fabs(est.owd_min_us - 10000.0) < 1.0, "min = 10ms");
    TEST_ASSERT(fabs(est.owd_max_us - 10000.0) < 1.0, "max = 10ms");
    TEST_ASSERT(est.sample_count == 1, "1 sample");

    /* Feed 100 more samples at 10000µs — EWMA should stay ≈ 10000 */
    for (int i = 0; i < 100; i++)
        probe_estimator_observe(pe, (uint64_t)i*100, (uint64_t)i*100 + 10000, 32);
    probe_estimator_snapshot(pe, &est);
    TEST_ASSERT(fabs(est.owd_us - 10000.0) < 200.0, "EWMA converges to 10ms");

    probe_estimator_reset(pe);
    TEST_ASSERT(!probe_estimator_has_samples(pe), "reset clears samples");

    probe_estimator_destroy(pe);
    TEST_PASS("probe_estimator OWD EWMA");
    return 0;
}

static int test_estimator_bandwidth(void) {
    printf("\n=== test_estimator_bandwidth ===\n");

    probe_estimator_t *pe = probe_estimator_create();

    /* Inject packets spaced 1ms apart, each 1000 bytes.
     * Expected BW ≈ 1000*8 / 0.001 = 8 Mbps */
    for (int i = 0; i < 10; i++) {
        uint64_t ts = (uint64_t)i * 1000; /* 1ms gap */
        probe_estimator_observe(pe, 0, ts, 1000);
    }
    probe_estimate_t est;
    probe_estimator_snapshot(pe, &est);
    /* After 10 observations BW estimate should be in roughly [4, 12] Mbps */
    TEST_ASSERT(est.bw_bps > 4000000.0 && est.bw_bps < 12000000.0,
                "BW estimate near 8 Mbps");

    probe_estimator_destroy(pe);
    TEST_PASS("probe_estimator bandwidth estimate");
    return 0;
}

static int test_estimator_null_guard(void) {
    printf("\n=== test_estimator_null_guard ===\n");

    TEST_ASSERT(probe_estimator_observe(NULL, 0, 1000, 32) == -1, "NULL → -1");
    /* recv < send should be rejected */
    probe_estimator_t *pe = probe_estimator_create();
    TEST_ASSERT(probe_estimator_observe(pe, 5000, 4000, 32) == -1, "recv < send → -1");
    probe_estimator_destroy(pe);

    TEST_PASS("probe_estimator null and invalid guards");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_probe_pkt_roundtrip();
    failures += test_probe_pkt_bad_magic();

    failures += test_sched_first_tick();
    failures += test_sched_burst_completion();
    failures += test_sched_set_interval();

    failures += test_estimator_owd();
    failures += test_estimator_bandwidth();
    failures += test_estimator_null_guard();

    printf("\n");
    if (failures == 0)
        printf("ALL BWPROBE TESTS PASSED\n");
    else
        printf("%d BWPROBE TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
