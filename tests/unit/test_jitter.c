/*
 * test_jitter.c — Unit tests for PHASE-50 Low-Latency Jitter Buffer
 *
 * Tests jitter_packet (encode/decode/ordering), jitter_buffer
 * (push/pop/peek/ordering/playout-delay/flush), and jitter_stats
 * (record/snapshot/reset/jitter-estimation).  No network hardware needed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/jitter/jitter_packet.h"
#include "../../src/jitter/jitter_buffer.h"
#include "../../src/jitter/jitter_stats.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── Packet helpers ──────────────────────────────────────────────── */

static jitter_packet_t make_pkt(uint32_t seq, uint32_t rtp, uint64_t capture) {
    jitter_packet_t p;
    memset(&p, 0, sizeof(p));
    p.seq_num    = seq;
    p.rtp_ts     = rtp;
    p.capture_us = capture;
    return p;
}

/* ── jitter_packet tests ─────────────────────────────────────────── */

static int test_packet_roundtrip(void) {
    printf("\n=== test_packet_roundtrip ===\n");

    jitter_packet_t orig = make_pkt(42, 900000, 1700000000ULL);
    orig.payload_type = 96;
    orig.flags        = 0x01;
    orig.payload_len  = 10;
    for (int i = 0; i < 10; i++) orig.payload[i] = (uint8_t)i;

    uint8_t buf[JITTER_PKT_HDR_SIZE + JITTER_MAX_PAYLOAD];
    int n = jitter_packet_encode(&orig, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "encode positive");

    jitter_packet_t decoded;
    int rc = jitter_packet_decode(buf, (size_t)n, &decoded);
    TEST_ASSERT(rc == 0, "decode ok");
    TEST_ASSERT(decoded.seq_num    == 42,            "seq_num preserved");
    TEST_ASSERT(decoded.rtp_ts     == 900000,        "rtp_ts preserved");
    TEST_ASSERT(decoded.capture_us == 1700000000ULL, "capture_us preserved");
    TEST_ASSERT(decoded.payload_len == 10,           "payload_len preserved");
    TEST_ASSERT(decoded.payload[5] == 5,             "payload data preserved");

    TEST_PASS("jitter_packet encode/decode round-trip");
    return 0;
}

static int test_packet_bad_magic(void) {
    printf("\n=== test_packet_bad_magic ===\n");

    uint8_t buf[JITTER_PKT_HDR_SIZE] = {0};
    jitter_packet_t p;
    TEST_ASSERT(jitter_packet_decode(buf, sizeof(buf), &p) == -1,
                "bad magic → -1");

    TEST_PASS("jitter_packet bad magic rejected");
    return 0;
}

static int test_packet_ordering(void) {
    printf("\n=== test_packet_ordering ===\n");

    /* Normal order */
    TEST_ASSERT( jitter_packet_before(0, 1),     "0 before 1");
    TEST_ASSERT( jitter_packet_before(100, 200),  "100 before 200");
    TEST_ASSERT(!jitter_packet_before(200, 100),  "200 not before 100");

    /* Wrap-around */
    TEST_ASSERT( jitter_packet_before(0xFFFFFFFE, 0), "wrap: 0xFFFFFFFE before 0");
    TEST_ASSERT(!jitter_packet_before(0, 0xFFFFFFFE),  "wrap: 0 not before 0xFFFFFFFE");

    /* Equal */
    TEST_ASSERT(!jitter_packet_before(5, 5), "equal: 5 not before 5");

    TEST_PASS("jitter_packet ordering (with wrap-around)");
    return 0;
}

static int test_packet_null_guards(void) {
    printf("\n=== test_packet_null_guards ===\n");

    uint8_t buf[64];
    jitter_packet_t p; memset(&p, 0, sizeof(p));
    TEST_ASSERT(jitter_packet_encode(NULL, buf, sizeof(buf)) == -1, "encode NULL pkt");
    TEST_ASSERT(jitter_packet_encode(&p, NULL, 0) == -1, "encode NULL buf");
    TEST_ASSERT(jitter_packet_decode(NULL, 0, &p) == -1, "decode NULL buf");

    TEST_PASS("jitter_packet NULL guards");
    return 0;
}

/* ── jitter_buffer tests ─────────────────────────────────────────── */

static int test_buffer_create(void) {
    printf("\n=== test_buffer_create ===\n");

    jitter_buffer_t *b = jitter_buffer_create(50000);
    TEST_ASSERT(b != NULL, "buffer created");
    TEST_ASSERT(jitter_buffer_count(b) == 0, "initial count 0");
    TEST_ASSERT(jitter_buffer_is_empty(b), "initial is_empty");

    jitter_buffer_destroy(b);
    jitter_buffer_destroy(NULL); /* must not crash */
    TEST_PASS("jitter_buffer create/destroy");
    return 0;
}

static int test_buffer_ordering(void) {
    printf("\n=== test_buffer_ordering ===\n");

    jitter_buffer_t *b = jitter_buffer_create(0); /* 0 delay — pop immediately */

    /* Push out-of-order: 3, 1, 2 */
    jitter_packet_t p3 = make_pkt(3, 0, 0);
    jitter_packet_t p1 = make_pkt(1, 0, 0);
    jitter_packet_t p2 = make_pkt(2, 0, 0);

    jitter_buffer_push(b, &p3);
    jitter_buffer_push(b, &p1);
    jitter_buffer_push(b, &p2);
    TEST_ASSERT(jitter_buffer_count(b) == 3, "3 packets enqueued");

    jitter_packet_t out;
    jitter_buffer_peek(b, &out);
    TEST_ASSERT(out.seq_num == 1, "peek returns lowest seq");

    /* Pop all and verify ascending order */
    jitter_buffer_pop(b, 100, &out);
    TEST_ASSERT(out.seq_num == 1, "pop first: seq 1");
    jitter_buffer_pop(b, 100, &out);
    TEST_ASSERT(out.seq_num == 2, "pop second: seq 2");
    jitter_buffer_pop(b, 100, &out);
    TEST_ASSERT(out.seq_num == 3, "pop third: seq 3");

    jitter_buffer_destroy(b);
    TEST_PASS("jitter_buffer sorted ordering");
    return 0;
}

static int test_buffer_playout_delay(void) {
    printf("\n=== test_buffer_playout_delay ===\n");

    uint64_t delay = 50000; /* 50ms */
    jitter_buffer_t *b = jitter_buffer_create(delay);

    jitter_packet_t p = make_pkt(1, 0, 1000000); /* capture at t=1s */
    jitter_buffer_push(b, &p);

    jitter_packet_t out;

    /* Not due yet: now = 1.03s (< 1.0 + 0.05 = 1.05s) */
    int rc = jitter_buffer_pop(b, 1030000, &out);
    TEST_ASSERT(rc == -1, "packet not due at t=1.03s");

    /* Due: now = 1.06s (> 1.05s) */
    rc = jitter_buffer_pop(b, 1060000, &out);
    TEST_ASSERT(rc == 0, "packet due at t=1.06s");
    TEST_ASSERT(out.seq_num == 1, "correct packet popped");

    jitter_buffer_destroy(b);
    TEST_PASS("jitter_buffer playout delay");
    return 0;
}

static int test_buffer_flush(void) {
    printf("\n=== test_buffer_flush ===\n");

    jitter_buffer_t *b = jitter_buffer_create(50000);
    for (int i = 0; i < 5; i++) {
        jitter_packet_t p = make_pkt((uint32_t)i, 0, (uint64_t)i*1000);
        jitter_buffer_push(b, &p);
    }
    TEST_ASSERT(jitter_buffer_count(b) == 5, "5 before flush");

    jitter_buffer_flush(b);
    TEST_ASSERT(jitter_buffer_is_empty(b), "empty after flush");

    jitter_buffer_destroy(b);
    TEST_PASS("jitter_buffer flush");
    return 0;
}

/* ── jitter_stats tests ──────────────────────────────────────────── */

static int test_stats_basic(void) {
    printf("\n=== test_stats_basic ===\n");

    jitter_stats_t *st = jitter_stats_create();
    TEST_ASSERT(st != NULL, "stats created");

    /* 3 packets: delays 100µs, 200µs, 300µs */
    jitter_stats_record_arrival(st, 0, 100, 0, 0);
    jitter_stats_record_arrival(st, 0, 200, 0, 0);
    jitter_stats_record_arrival(st, 0, 300, 1, 1); /* 1 late, 1 dropped */

    jitter_stats_snapshot_t snap;
    int rc = jitter_stats_snapshot(st, &snap);
    TEST_ASSERT(rc == 0, "snapshot ok");
    TEST_ASSERT(snap.packets_received == 3, "3 received");
    TEST_ASSERT(snap.packets_late     == 1, "1 late");
    TEST_ASSERT(snap.packets_dropped  == 1, "1 dropped");
    TEST_ASSERT(fabs(snap.avg_delay_us - 200.0) < 1.0, "avg delay ~200µs");
    TEST_ASSERT(snap.min_delay_us == 100.0, "min delay 100µs");
    TEST_ASSERT(snap.max_delay_us == 300.0, "max delay 300µs");

    jitter_stats_destroy(st);
    TEST_PASS("jitter_stats basic metrics");
    return 0;
}

static int test_stats_jitter_rfc3550(void) {
    printf("\n=== test_stats_jitter_rfc3550 ===\n");

    jitter_stats_t *st = jitter_stats_create();

    /* Constant 10ms delay → jitter = 0 */
    for (int i = 0; i < 20; i++)
        jitter_stats_record_arrival(st, (uint64_t)(i * 1000),
                                     (uint64_t)(i * 1000 + 10000), 0, 0);

    jitter_stats_snapshot_t snap;
    jitter_stats_snapshot(st, &snap);
    TEST_ASSERT(fabs(snap.jitter_us) < 1.0, "constant delay → jitter ≈ 0");

    /* Reset and feed variable delay: alternating 5ms and 15ms */
    jitter_stats_reset(st);
    for (int i = 0; i < 20; i++) {
        uint64_t recv = (uint64_t)(i * 1000) + (uint64_t)(((i % 2) == 0) ? 5000 : 15000);
        jitter_stats_record_arrival(st, (uint64_t)(i * 1000), recv, 0, 0);
    }
    jitter_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.jitter_us > 0.0, "variable delay → jitter > 0");

    jitter_stats_destroy(st);
    TEST_PASS("jitter_stats RFC 3550 estimator");
    return 0;
}

static int test_stats_reset(void) {
    printf("\n=== test_stats_reset ===\n");

    jitter_stats_t *st = jitter_stats_create();
    jitter_stats_record_arrival(st, 0, 1000, 0, 0);

    jitter_stats_reset(st);
    jitter_stats_snapshot_t snap;
    jitter_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.packets_received == 0, "reset clears count");
    TEST_ASSERT(snap.avg_delay_us == 0.0, "reset clears avg");

    jitter_stats_destroy(st);
    TEST_PASS("jitter_stats reset");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_packet_roundtrip();
    failures += test_packet_bad_magic();
    failures += test_packet_ordering();
    failures += test_packet_null_guards();

    failures += test_buffer_create();
    failures += test_buffer_ordering();
    failures += test_buffer_playout_delay();
    failures += test_buffer_flush();

    failures += test_stats_basic();
    failures += test_stats_jitter_rfc3550();
    failures += test_stats_reset();

    printf("\n");
    if (failures == 0)
        printf("ALL JITTER TESTS PASSED\n");
    else
        printf("%d JITTER TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
