/*
 * test_loss.c — Unit tests for PHASE-70 Packet Loss Estimator
 *
 * Tests loss_window (init/receive/loss_rate/reset/sliding),
 * loss_rate (init/receive/ewma/reset), and loss_stats
 * (record/burst_count/max_burst/snapshot/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "../../src/loss/loss_window.h"
#include "../../src/loss/loss_rate.h"
#include "../../src/loss/loss_stats.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

/* ── loss_window ─────────────────────────────────────────────────── */

static int test_window_no_loss(void) {
    printf("\n=== test_window_no_loss ===\n");

    loss_window_t w;
    TEST_ASSERT(lw_init(&w) == 0, "init ok");

    /* Receive 10 consecutive packets */
    for (int i = 0; i < 10; i++)
        lw_receive(&w, (uint16_t)i);

    double r = lw_loss_rate(&w);
    TEST_ASSERT(r == 0.0, "0 loss for consecutive packets");

    lw_reset(&w);
    TEST_ASSERT(!w.initialised, "reset clears initialised");
    TEST_ASSERT(lw_loss_rate(&w) == 0.0, "rate=0 after reset");

    TEST_PASS("loss_window consecutive / no loss / reset");
    return 0;
}

static int test_window_with_loss(void) {
    printf("\n=== test_window_with_loss ===\n");

    loss_window_t w;
    lw_init(&w);

    /* Receive 0, skip 1, receive 2 → seq 1 lost */
    lw_receive(&w, 0);
    lw_receive(&w, 2);   /* seq 1 skipped → advances window, marks 1 lost */

    double r = lw_loss_rate(&w);
    TEST_ASSERT(r > 0.0, "loss rate > 0 when packet skipped");

    TEST_PASS("loss_window skip → loss detected");
    return 0;
}

/* ── loss_rate ───────────────────────────────────────────────────── */

static int test_loss_rate_ewma(void) {
    printf("\n=== test_loss_rate_ewma ===\n");

    loss_rate_t lr;
    TEST_ASSERT(lr_rate_init(&lr) == 0, "init ok");
    TEST_ASSERT(!lr.ready, "not ready initially");

    /* Receive some consecutive packets */
    for (int i = 0; i < 10; i++)
        lr_rate_receive(&lr, (uint16_t)i);
    TEST_ASSERT(lr.ready, "ready after first receive");

    double ewma = lr_rate_ewma(&lr);
    TEST_ASSERT(ewma >= 0.0 && ewma <= 1.0, "ewma in [0,1]");

    lr_rate_reset(&lr);
    TEST_ASSERT(!lr.ready, "reset clears ready");
    TEST_ASSERT(lr_rate_ewma(&lr) == 0.0, "ewma=0 after reset");

    TEST_PASS("loss_rate receive / ewma / ready / reset");
    return 0;
}

/* ── loss_stats ──────────────────────────────────────────────────── */

static int test_loss_stats(void) {
    printf("\n=== test_loss_stats ===\n");

    loss_stats_t *st = loss_stats_create();
    TEST_ASSERT(st != NULL, "created");

    /* Sequence: received, received, LOST LOST LOST, received, LOST, received */
    loss_stats_record(st, 0);  /* recv */
    loss_stats_record(st, 0);  /* recv */
    loss_stats_record(st, 1);  /* lost burst 1 start */
    loss_stats_record(st, 1);  /* lost */
    loss_stats_record(st, 1);  /* lost burst 1 end (length 3) */
    loss_stats_record(st, 0);  /* recv */
    loss_stats_record(st, 1);  /* lost burst 2 (length 1) */
    loss_stats_record(st, 0);  /* recv */

    loss_stats_snapshot_t snap;
    TEST_ASSERT(loss_stats_snapshot(st, &snap) == 0, "snapshot ok");
    TEST_ASSERT(snap.total_sent == 8, "8 packets");
    TEST_ASSERT(snap.total_lost == 4, "4 lost");
    TEST_ASSERT(snap.burst_count == 2, "2 bursts");
    TEST_ASSERT(snap.max_burst   == 3, "max burst = 3");
    TEST_ASSERT(fabs(snap.loss_pct - 50.0) < 0.01, "50% loss");

    loss_stats_reset(st);
    loss_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.total_sent == 0, "reset ok");

    loss_stats_destroy(st);
    TEST_PASS("loss_stats record/burst/max_burst/snapshot/reset");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_window_no_loss();
    failures += test_window_with_loss();
    failures += test_loss_rate_ewma();
    failures += test_loss_stats();

    printf("\n");
    if (failures == 0) printf("ALL LOSS TESTS PASSED\n");
    else               printf("%d LOSS TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
