/*
 * test_drainq.c — Unit tests for PHASE-82 Drain Queue
 *
 * Tests dq_queue (create/enqueue/dequeue/drain_all/count/clear/
 * seq-increment/cap) and dq_stats (enqueue/drain/drop/peak/
 * snapshot/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../../src/drainq/dq_entry.h"
#include "../../src/drainq/dq_queue.h"
#include "../../src/drainq/dq_stats.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── dq_queue ────────────────────────────────────────────────────── */

static int test_queue_basic(void) {
    printf("\n=== test_queue_basic ===\n");

    dq_queue_t *q = dq_queue_create();
    TEST_ASSERT(q != NULL, "created");
    TEST_ASSERT(dq_queue_count(q) == 0, "initially empty");

    /* Dequeue from empty */
    dq_entry_t e;
    TEST_ASSERT(dq_queue_dequeue(q, &e) == -1, "dequeue empty → -1");

    /* Enqueue 3 entries */
    char payload1[] = "hello";
    char payload2[] = "world";
    char payload3[] = "!";

    dq_entry_t in = { 0, payload1, sizeof(payload1), 0 };
    TEST_ASSERT(dq_queue_enqueue(q, &in) == 0, "enqueue 1");
    in.data = payload2; in.data_len = sizeof(payload2);
    TEST_ASSERT(dq_queue_enqueue(q, &in) == 0, "enqueue 2");
    in.data = payload3; in.data_len = sizeof(payload3);
    TEST_ASSERT(dq_queue_enqueue(q, &in) == 0, "enqueue 3");
    TEST_ASSERT(dq_queue_count(q) == 3, "count = 3");

    /* Dequeue in FIFO order; seq numbers must be ascending */
    uint64_t prev_seq = UINT64_MAX;
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT(dq_queue_dequeue(q, &e) == 0, "dequeue ok");
        if (prev_seq != UINT64_MAX)
            TEST_ASSERT(e.seq == prev_seq + 1, "seq is ascending");
        prev_seq = e.seq;
    }
    TEST_ASSERT(dq_queue_count(q) == 0, "empty after 3 dequeues");

    dq_queue_destroy(q);
    TEST_PASS("dq_queue enqueue/dequeue/seq/FIFO");
    return 0;
}

static int g_drain_count = 0;
static void drain_cb(const dq_entry_t *e, void *user) { (void)e; (void)user; g_drain_count++; }

static int test_queue_drain_all(void) {
    printf("\n=== test_queue_drain_all ===\n");

    dq_queue_t *q = dq_queue_create();
    for (int i = 0; i < 5; i++) {
        dq_entry_t e = { 0, NULL, 0, 0 };
        dq_queue_enqueue(q, &e);
    }
    TEST_ASSERT(dq_queue_count(q) == 5, "5 enqueued");

    g_drain_count = 0;
    int n = dq_queue_drain_all(q, drain_cb, NULL);
    TEST_ASSERT(n == 5, "drain_all returns 5");
    TEST_ASSERT(g_drain_count == 5, "cb called 5 times");
    TEST_ASSERT(dq_queue_count(q) == 0, "empty after drain_all");

    dq_queue_destroy(q);
    TEST_PASS("dq_queue drain_all");
    return 0;
}

static int test_queue_clear(void) {
    printf("\n=== test_queue_clear ===\n");

    dq_queue_t *q = dq_queue_create();
    for (int i = 0; i < 10; i++) {
        dq_entry_t e = { 0, NULL, 0, 0 };
        dq_queue_enqueue(q, &e);
    }
    TEST_ASSERT(dq_queue_count(q) == 10, "10 before clear");
    dq_queue_clear(q);
    TEST_ASSERT(dq_queue_count(q) == 0, "0 after clear");

    dq_queue_destroy(q);
    TEST_PASS("dq_queue clear");
    return 0;
}

static int test_queue_full(void) {
    printf("\n=== test_queue_full ===\n");

    dq_queue_t *q = dq_queue_create();
    for (int i = 0; i < DQ_MAX_ENTRIES; i++) {
        dq_entry_t e = { 0, NULL, 0, 0 };
        TEST_ASSERT(dq_queue_enqueue(q, &e) == 0, "enqueue within cap");
    }
    dq_entry_t extra = { 0, NULL, 0, 0 };
    TEST_ASSERT(dq_queue_enqueue(q, &extra) == -1, "full → -1");

    dq_queue_destroy(q);
    TEST_PASS("dq_queue capacity enforcement");
    return 0;
}

/* ── dq_stats ────────────────────────────────────────────────────── */

static int test_dq_stats(void) {
    printf("\n=== test_dq_stats ===\n");

    dq_stats_t *st = dq_stats_create();
    TEST_ASSERT(st != NULL, "created");

    dq_stats_record_enqueue(st, 3);   /* peak = 3 */
    dq_stats_record_enqueue(st, 7);   /* peak = 7 */
    dq_stats_record_enqueue(st, 2);
    dq_stats_record_drain(st);
    dq_stats_record_drain(st);
    dq_stats_record_drop(st);
    dq_stats_record_drop(st);

    dq_stats_snapshot_t snap;
    TEST_ASSERT(dq_stats_snapshot(st, &snap) == 0, "snapshot ok");
    TEST_ASSERT(snap.enqueued == 3, "3 enqueued");
    TEST_ASSERT(snap.drained  == 2, "2 drained");
    TEST_ASSERT(snap.dropped  == 2, "2 dropped");
    TEST_ASSERT(snap.peak     == 7, "peak = 7");

    dq_stats_reset(st);
    dq_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.enqueued == 0, "reset ok");

    dq_stats_destroy(st);
    TEST_PASS("dq_stats enqueue/drain/drop/peak/snapshot/reset");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_queue_basic();
    failures += test_queue_drain_all();
    failures += test_queue_clear();
    failures += test_queue_full();
    failures += test_dq_stats();

    printf("\n");
    if (failures == 0) printf("ALL DRAINQ TESTS PASSED\n");
    else               printf("%d DRAINQ TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
