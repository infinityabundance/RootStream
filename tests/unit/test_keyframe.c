/*
 * test_keyframe.c — Unit tests for PHASE-57 IDR/Keyframe Request Handler
 *
 * Tests kfr_message (encode/decode/bad-magic/type-names),
 * kfr_handler (forward/suppress/cooldown/urgent/flush/set-cooldown),
 * and kfr_stats (record/snapshot/suppression-rate/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/keyframe/kfr_message.h"
#include "../../src/keyframe/kfr_handler.h"
#include "../../src/keyframe/kfr_stats.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

/* ── kfr_message tests ───────────────────────────────────────────── */

static int test_kfr_msg_roundtrip(void) {
    printf("\n=== test_kfr_msg_roundtrip ===\n");

    kfr_message_t msg = {
        .type         = KFR_TYPE_PLI,
        .priority     = 0,
        .seq          = 42,
        .ssrc         = 0xDEADBEEFU,
        .timestamp_us = 1234567890ULL,
    };

    uint8_t buf[KFR_MSG_SIZE];
    int n = kfr_message_encode(&msg, buf, sizeof(buf));
    TEST_ASSERT(n == KFR_MSG_SIZE, "encoded size = KFR_MSG_SIZE");

    kfr_message_t dec;
    int rc = kfr_message_decode(buf, sizeof(buf), &dec);
    TEST_ASSERT(rc == 0, "decode ok");
    TEST_ASSERT(dec.type         == KFR_TYPE_PLI,      "type PLI");
    TEST_ASSERT(dec.seq          == 42,                 "seq");
    TEST_ASSERT(dec.ssrc         == 0xDEADBEEFU,        "ssrc");
    TEST_ASSERT(dec.timestamp_us == 1234567890ULL,      "timestamp");

    TEST_PASS("kfr_message PLI round-trip");
    return 0;
}

static int test_kfr_msg_fir(void) {
    printf("\n=== test_kfr_msg_fir ===\n");

    kfr_message_t msg = { .type = KFR_TYPE_FIR, .priority = 1,
                           .seq = 1, .ssrc = 1, .timestamp_us = 0 };
    uint8_t buf[KFR_MSG_SIZE];
    kfr_message_encode(&msg, buf, sizeof(buf));

    kfr_message_t dec;
    TEST_ASSERT(kfr_message_decode(buf, sizeof(buf), &dec) == 0, "FIR decode ok");
    TEST_ASSERT(dec.type == KFR_TYPE_FIR, "type FIR");
    TEST_ASSERT(dec.priority == 1, "priority urgent");

    TEST_PASS("kfr_message FIR round-trip");
    return 0;
}

static int test_kfr_msg_bad_magic(void) {
    printf("\n=== test_kfr_msg_bad_magic ===\n");

    uint8_t buf[KFR_MSG_SIZE] = {0};
    kfr_message_t dec;
    TEST_ASSERT(kfr_message_decode(buf, sizeof(buf), &dec) == -1, "bad magic → -1");

    TEST_PASS("kfr_message bad magic rejected");
    return 0;
}

static int test_kfr_type_names(void) {
    printf("\n=== test_kfr_type_names ===\n");

    TEST_ASSERT(strcmp(kfr_type_name(KFR_TYPE_PLI), "PLI") == 0, "PLI");
    TEST_ASSERT(strcmp(kfr_type_name(KFR_TYPE_FIR), "FIR") == 0, "FIR");
    TEST_ASSERT(strcmp(kfr_type_name((kfr_type_t)99), "UNKNOWN") == 0, "unknown");

    TEST_PASS("kfr_message type names");
    return 0;
}

/* ── kfr_handler tests ───────────────────────────────────────────── */

static kfr_message_t make_kfr(uint32_t ssrc, uint8_t prio) {
    kfr_message_t m; memset(&m, 0, sizeof(m));
    m.type = KFR_TYPE_PLI; m.ssrc = ssrc; m.priority = prio;
    return m;
}

static int test_handler_forward(void) {
    printf("\n=== test_handler_forward ===\n");

    kfr_handler_t *h = kfr_handler_create(KFR_DEFAULT_COOLDOWN_US);
    TEST_ASSERT(h != NULL, "handler created");

    kfr_message_t m = make_kfr(0x1000, 0);
    kfr_decision_t d = kfr_handler_submit(h, &m, 0);
    TEST_ASSERT(d == KFR_DECISION_FORWARD, "first request forwarded");

    /* Immediately again (within cooldown) → suppress */
    d = kfr_handler_submit(h, &m, 1000);
    TEST_ASSERT(d == KFR_DECISION_SUPPRESS, "immediate dup suppressed");

    /* After cooldown → forward again */
    d = kfr_handler_submit(h, &m, KFR_DEFAULT_COOLDOWN_US + 1);
    TEST_ASSERT(d == KFR_DECISION_FORWARD, "after cooldown: forwarded");

    kfr_handler_destroy(h);
    TEST_PASS("kfr_handler forward/suppress/cooldown");
    return 0;
}

static int test_handler_urgent(void) {
    printf("\n=== test_handler_urgent ===\n");

    kfr_handler_t *h = kfr_handler_create(KFR_DEFAULT_COOLDOWN_US);

    kfr_message_t m = make_kfr(0x2000, 0);
    kfr_handler_submit(h, &m, 0);  /* forward normal */

    /* Urgent request within cooldown → still forwarded */
    m.priority = 1;
    kfr_decision_t d = kfr_handler_submit(h, &m, 1000);
    TEST_ASSERT(d == KFR_DECISION_FORWARD, "urgent bypasses cooldown");

    kfr_handler_destroy(h);
    TEST_PASS("kfr_handler urgent request bypass");
    return 0;
}

static int test_handler_flush(void) {
    printf("\n=== test_handler_flush ===\n");

    kfr_handler_t *h = kfr_handler_create(KFR_DEFAULT_COOLDOWN_US);

    kfr_message_t m = make_kfr(0x3000, 0);
    kfr_handler_submit(h, &m, 0);  /* forward */

    /* Before cooldown → suppressed */
    TEST_ASSERT(kfr_handler_submit(h, &m, 100) == KFR_DECISION_SUPPRESS,
                "suppressed before flush");

    /* Flush → next request forwarded regardless of cooldown */
    kfr_handler_flush_ssrc(h, 0x3000);
    TEST_ASSERT(kfr_handler_submit(h, &m, 101) == KFR_DECISION_FORWARD,
                "forwarded after flush");

    kfr_handler_destroy(h);
    TEST_PASS("kfr_handler flush_ssrc");
    return 0;
}

static int test_handler_multi_ssrc(void) {
    printf("\n=== test_handler_multi_ssrc ===\n");

    kfr_handler_t *h = kfr_handler_create(KFR_DEFAULT_COOLDOWN_US);

    kfr_message_t m1 = make_kfr(0xAAAA, 0);
    kfr_message_t m2 = make_kfr(0xBBBB, 0);

    TEST_ASSERT(kfr_handler_submit(h, &m1, 0) == KFR_DECISION_FORWARD, "ssrc1 fwd");
    TEST_ASSERT(kfr_handler_submit(h, &m2, 0) == KFR_DECISION_FORWARD, "ssrc2 fwd (independent)");
    TEST_ASSERT(kfr_handler_submit(h, &m1, 100) == KFR_DECISION_SUPPRESS, "ssrc1 dup");
    TEST_ASSERT(kfr_handler_submit(h, &m2, 100) == KFR_DECISION_SUPPRESS, "ssrc2 dup");

    kfr_handler_destroy(h);
    TEST_PASS("kfr_handler independent per-SSRC cooldown");
    return 0;
}

/* ── kfr_stats tests ─────────────────────────────────────────────── */

static int test_kfr_stats(void) {
    printf("\n=== test_kfr_stats ===\n");

    kfr_stats_t *st = kfr_stats_create();
    TEST_ASSERT(st != NULL, "stats created");

    kfr_stats_record(st, 1, 0); /* forwarded, normal */
    kfr_stats_record(st, 0, 0); /* suppressed */
    kfr_stats_record(st, 0, 0);
    kfr_stats_record(st, 1, 1); /* forwarded urgent */

    kfr_stats_snapshot_t snap;
    int rc = kfr_stats_snapshot(st, &snap);
    TEST_ASSERT(rc == 0, "snapshot ok");
    TEST_ASSERT(snap.requests_received   == 4, "4 received");
    TEST_ASSERT(snap.requests_forwarded  == 2, "2 forwarded");
    TEST_ASSERT(snap.requests_suppressed == 2, "2 suppressed");
    TEST_ASSERT(snap.urgent_requests     == 1, "1 urgent");
    TEST_ASSERT(fabs(snap.suppression_rate - 0.5) < 0.01, "suppression rate 50%");

    kfr_stats_reset(st);
    kfr_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.requests_received == 0, "reset clears stats");

    kfr_stats_destroy(st);
    TEST_PASS("kfr_stats record/snapshot/suppression-rate/reset");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_kfr_msg_roundtrip();
    failures += test_kfr_msg_fir();
    failures += test_kfr_msg_bad_magic();
    failures += test_kfr_type_names();

    failures += test_handler_forward();
    failures += test_handler_urgent();
    failures += test_handler_flush();
    failures += test_handler_multi_ssrc();

    failures += test_kfr_stats();

    printf("\n");
    if (failures == 0)
        printf("ALL KEYFRAME TESTS PASSED\n");
    else
        printf("%d KEYFRAME TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
