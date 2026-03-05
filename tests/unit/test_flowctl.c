/*
 * test_flowctl.c — Unit tests for PHASE-79 Flow Controller
 *
 * Tests fc_params (init/invalid), fc_engine (create/consume/
 * can_send/replenish/credit/reset/cap), and fc_stats
 * (send/drop/stall/replenish/snapshot/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../../src/flowctl/fc_params.h"
#include "../../src/flowctl/fc_engine.h"
#include "../../src/flowctl/fc_stats.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── fc_params ───────────────────────────────────────────────────── */

static int test_params_init(void) {
    printf("\n=== test_params_init ===\n");

    fc_params_t p;
    TEST_ASSERT(fc_params_init(&p, 65536, 8192, 32768, 512) == 0, "init ok");
    TEST_ASSERT(p.window_bytes == 65536, "window_bytes");
    TEST_ASSERT(p.send_budget  ==  8192, "send_budget");
    TEST_ASSERT(p.recv_window  == 32768, "recv_window");
    TEST_ASSERT(p.credit_step  ==   512, "credit_step");

    TEST_ASSERT(fc_params_init(NULL, 1, 1, 1, 1) == -1, "NULL → -1");
    TEST_ASSERT(fc_params_init(&p, 0, 1, 1, 1)   == -1, "window=0 → -1");
    TEST_ASSERT(fc_params_init(&p, 1, 0, 1, 1)   == -1, "budget=0 → -1");

    TEST_PASS("fc_params init / invalid guard");
    return 0;
}

/* ── fc_engine ───────────────────────────────────────────────────── */

static int test_engine_basic(void) {
    printf("\n=== test_engine_basic ===\n");

    fc_params_t p;
    fc_params_init(&p, 1000, 500, 1000, 100);

    fc_engine_t *e = fc_engine_create(&p);
    TEST_ASSERT(e != NULL, "created");
    TEST_ASSERT(fc_engine_credit(e) == 500, "initial credit = send_budget");

    /* can_send / consume */
    TEST_ASSERT(fc_engine_can_send(e, 500), "can_send 500");
    TEST_ASSERT(!fc_engine_can_send(e, 501), "cannot send 501");
    TEST_ASSERT(fc_engine_consume(e, 200) == 0, "consume 200 ok");
    TEST_ASSERT(fc_engine_credit(e) == 300, "credit = 300");
    TEST_ASSERT(fc_engine_consume(e, 301) == -1, "consume 301 → -1");
    TEST_ASSERT(fc_engine_credit(e) == 300, "credit unchanged after fail");

    /* replenish — credit_step is 100, so add max(100, requested) */
    uint32_t c = fc_engine_replenish(e, 50);   /* 50 < credit_step → use 100 */
    TEST_ASSERT(c == 400, "replenish 50 → credit_step enforced → 400");

    /* replenish capped at window_bytes */
    fc_engine_replenish(e, 700);               /* 400 + 700 > 1000, cap */
    TEST_ASSERT(fc_engine_credit(e) == 1000, "replenish capped at window");

    /* reset */
    fc_engine_consume(e, 300);
    fc_engine_reset(e);
    TEST_ASSERT(fc_engine_credit(e) == 500, "reset restores send_budget");

    fc_engine_destroy(e);
    TEST_PASS("fc_engine consume/replenish/cap/reset");
    return 0;
}

/* ── fc_stats ────────────────────────────────────────────────────── */

static int test_fc_stats(void) {
    printf("\n=== test_fc_stats ===\n");

    fc_stats_t *st = fc_stats_create();
    TEST_ASSERT(st != NULL, "created");

    fc_stats_record_send(st, 1000);
    fc_stats_record_send(st, 500);
    fc_stats_record_drop(st, 200);
    fc_stats_record_stall(st);
    fc_stats_record_stall(st);
    fc_stats_record_replenish(st);

    fc_stats_snapshot_t snap;
    TEST_ASSERT(fc_stats_snapshot(st, &snap) == 0, "snapshot ok");
    TEST_ASSERT(snap.bytes_sent      == 1500, "1500 bytes sent");
    TEST_ASSERT(snap.bytes_dropped   ==  200, "200 bytes dropped");
    TEST_ASSERT(snap.stalls          ==    2, "2 stalls");
    TEST_ASSERT(snap.replenish_count ==    1, "1 replenish");

    fc_stats_reset(st);
    fc_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.bytes_sent == 0, "reset ok");

    fc_stats_destroy(st);
    TEST_PASS("fc_stats send/drop/stall/replenish/snapshot/reset");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_params_init();
    failures += test_engine_basic();
    failures += test_fc_stats();

    printf("\n");
    if (failures == 0) printf("ALL FLOWCTL TESTS PASSED\n");
    else               printf("%d FLOWCTL TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
