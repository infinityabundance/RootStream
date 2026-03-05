/*
 * test_plc.c — Unit tests for PHASE-51 Packet Loss Concealment
 *
 * Tests plc_frame (encode/decode/is_silent), plc_history (push/get/
 * wrap-around), plc_conceal (zero/repeat/fade-out), and plc_stats
 * (record received/lost, loss rate, burst tracking).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/plc/plc_frame.h"
#include "../../src/plc/plc_history.h"
#include "../../src/plc/plc_conceal.h"
#include "../../src/plc/plc_stats.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── Helpers ─────────────────────────────────────────────────────── */

static plc_frame_t make_frame(uint32_t seq, uint16_t ch, uint16_t samp) {
    plc_frame_t f;
    memset(&f, 0, sizeof(f));
    f.seq_num    = seq;
    f.channels   = ch;
    f.num_samples = samp;
    f.sample_rate = 48000;
    for (int i = 0; i < (int)(ch * samp); i++)
        f.samples[i] = (int16_t)(i % 256 - 128);
    return f;
}

/* ── plc_frame tests ─────────────────────────────────────────────── */

static int test_frame_roundtrip(void) {
    printf("\n=== test_frame_roundtrip ===\n");

    plc_frame_t orig = make_frame(42, 2, 480);
    uint8_t buf[PLC_FRAME_HDR_SIZE + PLC_MAX_FRAME_SAMPLES * 2];
    int n = plc_frame_encode(&orig, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "encode positive");

    plc_frame_t dec;
    int rc = plc_frame_decode(buf, (size_t)n, &dec);
    TEST_ASSERT(rc == 0, "decode ok");
    TEST_ASSERT(dec.seq_num     == 42,    "seq_num");
    TEST_ASSERT(dec.channels    == 2,     "channels");
    TEST_ASSERT(dec.num_samples == 480,   "num_samples");
    TEST_ASSERT(dec.sample_rate == 48000, "sample_rate");
    TEST_ASSERT(dec.samples[0]  == orig.samples[0], "sample data");

    TEST_PASS("plc_frame encode/decode round-trip");
    return 0;
}

static int test_frame_bad_magic(void) {
    printf("\n=== test_frame_bad_magic ===\n");

    uint8_t buf[PLC_FRAME_HDR_SIZE] = {0};
    plc_frame_t f;
    TEST_ASSERT(plc_frame_decode(buf, sizeof(buf), &f) == -1,
                "bad magic → -1");

    TEST_PASS("plc_frame bad magic rejected");
    return 0;
}

static int test_frame_is_silent(void) {
    printf("\n=== test_frame_is_silent ===\n");

    plc_frame_t f; memset(&f, 0, sizeof(f));
    f.channels = 1; f.num_samples = 4;
    TEST_ASSERT(plc_frame_is_silent(&f), "zero frame is silent");

    f.samples[2] = 100;
    TEST_ASSERT(!plc_frame_is_silent(&f), "non-zero not silent");
    TEST_ASSERT(plc_frame_is_silent(NULL), "NULL → silent");

    TEST_PASS("plc_frame is_silent");
    return 0;
}

static int test_frame_null_guards(void) {
    printf("\n=== test_frame_null_guards ===\n");

    uint8_t buf[64];
    plc_frame_t f = make_frame(1, 1, 10);
    TEST_ASSERT(plc_frame_encode(NULL, buf, sizeof(buf)) == -1, "encode NULL frame");
    TEST_ASSERT(plc_frame_encode(&f, NULL, 0) == -1, "encode NULL buf");
    TEST_ASSERT(plc_frame_decode(NULL, 0, &f) == -1, "decode NULL buf");

    TEST_PASS("plc_frame NULL guards");
    return 0;
}

/* ── plc_history tests ───────────────────────────────────────────── */

static int test_history_push_get(void) {
    printf("\n=== test_history_push_get ===\n");

    plc_history_t *h = plc_history_create();
    TEST_ASSERT(h != NULL, "history created");
    TEST_ASSERT(plc_history_is_empty(h), "initially empty");

    plc_frame_t f1 = make_frame(1, 1, 10);
    plc_frame_t f2 = make_frame(2, 1, 10);
    plc_history_push(h, &f1);
    plc_history_push(h, &f2);
    TEST_ASSERT(plc_history_count(h) == 2, "count 2");

    plc_frame_t last;
    int rc = plc_history_get_last(h, &last);
    TEST_ASSERT(rc == 0, "get_last ok");
    TEST_ASSERT(last.seq_num == 2, "last is f2 (newest)");

    plc_frame_t prev;
    rc = plc_history_get(h, 1, &prev);
    TEST_ASSERT(rc == 0, "get age=1 ok");
    TEST_ASSERT(prev.seq_num == 1, "age 1 is f1");

    plc_history_destroy(h);
    TEST_PASS("plc_history push/get");
    return 0;
}

static int test_history_wraparound(void) {
    printf("\n=== test_history_wraparound ===\n");

    plc_history_t *h = plc_history_create();

    /* Fill beyond DEPTH to test wrap-around */
    for (int i = 0; i < PLC_HISTORY_DEPTH + 3; i++) {
        plc_frame_t f = make_frame((uint32_t)i, 1, 10);
        plc_history_push(h, &f);
    }
    TEST_ASSERT(plc_history_count(h) == PLC_HISTORY_DEPTH, "count capped at DEPTH");

    plc_frame_t newest;
    plc_history_get_last(h, &newest);
    TEST_ASSERT(newest.seq_num == (uint32_t)(PLC_HISTORY_DEPTH + 2),
                "newest frame is correct");

    plc_history_clear(h);
    TEST_ASSERT(plc_history_is_empty(h), "empty after clear");

    plc_history_destroy(h);
    TEST_PASS("plc_history wrap-around");
    return 0;
}

/* ── plc_conceal tests ───────────────────────────────────────────── */

static int test_conceal_zero(void) {
    printf("\n=== test_conceal_zero ===\n");

    plc_history_t *h = plc_history_create();
    plc_frame_t ref = make_frame(5, 2, 100);
    plc_history_push(h, &ref);

    plc_frame_t out;
    int rc = plc_conceal(h, PLC_STRATEGY_ZERO, 1, PLC_FADE_FACTOR_DEFAULT,
                          NULL, &out);
    TEST_ASSERT(rc == 0, "zero conceal ok");
    TEST_ASSERT(plc_frame_is_silent(&out), "output is silent");
    TEST_ASSERT(out.channels == 2, "metadata preserved (channels)");

    plc_history_destroy(h);
    TEST_PASS("plc_conceal ZERO strategy");
    return 0;
}

static int test_conceal_repeat(void) {
    printf("\n=== test_conceal_repeat ===\n");

    plc_history_t *h = plc_history_create();
    plc_frame_t ref = make_frame(10, 1, 20);
    ref.samples[0] = 1000;
    ref.samples[1] = -500;
    plc_history_push(h, &ref);

    plc_frame_t out;
    int rc = plc_conceal(h, PLC_STRATEGY_REPEAT, 1, PLC_FADE_FACTOR_DEFAULT,
                          NULL, &out);
    TEST_ASSERT(rc == 0, "repeat conceal ok");
    TEST_ASSERT(out.samples[0] == 1000 && out.samples[1] == -500,
                "samples copied from last frame");

    plc_history_destroy(h);
    TEST_PASS("plc_conceal REPEAT strategy");
    return 0;
}

static int test_conceal_fade_out(void) {
    printf("\n=== test_conceal_fade_out ===\n");

    plc_history_t *h = plc_history_create();
    plc_frame_t ref = make_frame(20, 1, 10);
    for (int i = 0; i < 10; i++) ref.samples[i] = 10000;
    plc_history_push(h, &ref);

    /* After 1 loss at 0.5 fade: amplitude = 0.5^1 = 0.5 */
    plc_frame_t out1;
    plc_conceal(h, PLC_STRATEGY_FADE_OUT, 1, 0.5f, NULL, &out1);
    TEST_ASSERT(out1.samples[0] == 5000, "1st loss faded to 50%");

    /* After 2 consecutive losses at 0.5 fade: amplitude = 0.5^2 = 0.25 */
    plc_frame_t out2;
    plc_conceal(h, PLC_STRATEGY_FADE_OUT, 2, 0.5f, NULL, &out2);
    TEST_ASSERT(out2.samples[0] == 2500, "2nd loss faded to 25%");

    plc_history_destroy(h);
    TEST_PASS("plc_conceal FADE_OUT strategy");
    return 0;
}

static int test_conceal_null_guard(void) {
    printf("\n=== test_conceal_null_guard ===\n");

    TEST_ASSERT(plc_conceal(NULL, PLC_STRATEGY_ZERO, 1, 0.9f, NULL, NULL) == -1,
                "NULL out → -1");

    TEST_PASS("plc_conceal NULL guard");
    return 0;
}

static int test_conceal_strategy_names(void) {
    printf("\n=== test_conceal_strategy_names ===\n");

    TEST_ASSERT(strcmp(plc_strategy_name(PLC_STRATEGY_ZERO), "zero") == 0,
                "zero name");
    TEST_ASSERT(strcmp(plc_strategy_name(PLC_STRATEGY_REPEAT), "repeat") == 0,
                "repeat name");
    TEST_ASSERT(strcmp(plc_strategy_name(PLC_STRATEGY_FADE_OUT), "fade_out") == 0,
                "fade_out name");
    TEST_ASSERT(strcmp(plc_strategy_name((plc_strategy_t)99), "unknown") == 0,
                "unknown name");

    TEST_PASS("plc_conceal strategy names");
    return 0;
}

/* ── plc_stats tests ─────────────────────────────────────────────── */

static int test_stats_basic(void) {
    printf("\n=== test_stats_basic ===\n");

    plc_stats_t *st = plc_stats_create();
    TEST_ASSERT(st != NULL, "stats created");

    /* 10 received, 3 lost (1 burst) */
    for (int i = 0; i < 10; i++) plc_stats_record_received(st);
    plc_stats_record_lost(st, 1); /* new burst */
    plc_stats_record_lost(st, 0);
    plc_stats_record_lost(st, 0);

    plc_stats_snapshot_t snap;
    int rc = plc_stats_snapshot(st, &snap);
    TEST_ASSERT(rc == 0, "snapshot ok");
    TEST_ASSERT(snap.frames_received    == 10, "10 received");
    TEST_ASSERT(snap.frames_lost        == 3,  "3 lost");
    TEST_ASSERT(snap.concealment_events == 1,  "1 burst");
    TEST_ASSERT(snap.max_consecutive_loss == 3, "max run 3");

    plc_stats_destroy(st);
    TEST_PASS("plc_stats basic");
    return 0;
}

static int test_stats_loss_rate(void) {
    printf("\n=== test_stats_loss_rate ===\n");

    plc_stats_t *st = plc_stats_create();

    /* 50% loss: alternating recv / lost */
    for (int i = 0; i < 20; i++) {
        plc_stats_record_received(st);
        plc_stats_record_lost(st, 1);
    }

    plc_stats_snapshot_t snap;
    plc_stats_snapshot(st, &snap);
    TEST_ASSERT(fabs(snap.loss_rate - 0.5) < 0.01, "loss rate ~0.5");

    plc_stats_reset(st);
    plc_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.frames_received == 0, "reset clears count");
    TEST_ASSERT(snap.loss_rate == 0.0,     "reset clears loss rate");

    plc_stats_destroy(st);
    TEST_PASS("plc_stats loss rate");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_frame_roundtrip();
    failures += test_frame_bad_magic();
    failures += test_frame_is_silent();
    failures += test_frame_null_guards();

    failures += test_history_push_get();
    failures += test_history_wraparound();

    failures += test_conceal_zero();
    failures += test_conceal_repeat();
    failures += test_conceal_fade_out();
    failures += test_conceal_null_guard();
    failures += test_conceal_strategy_names();

    failures += test_stats_basic();
    failures += test_stats_loss_rate();

    printf("\n");
    if (failures == 0)
        printf("ALL PLC TESTS PASSED\n");
    else
        printf("%d PLC TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
