/*
 * test_pqueue.c — Unit tests for PHASE-77 Priority Queue
 *
 * Tests pq_heap (create/push/pop/peek/count/clear/heap-order/
 * full-guard), and pq_stats (push/pop/overflow/peak/snapshot/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../../src/pqueue/pq_entry.h"
#include "../../src/pqueue/pq_heap.h"
#include "../../src/pqueue/pq_stats.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── pq_heap ─────────────────────────────────────────────────────── */

static int test_heap_create(void) {
    printf("\n=== test_heap_create ===\n");

    pq_heap_t *h = pq_heap_create();
    TEST_ASSERT(h != NULL, "created");
    TEST_ASSERT(pq_heap_count(h) == 0, "initially empty");

    pq_entry_t out;
    TEST_ASSERT(pq_heap_pop(h, &out)  == -1, "pop empty → -1");
    TEST_ASSERT(pq_heap_peek(h, &out) == -1, "peek empty → -1");

    pq_heap_destroy(h);
    TEST_PASS("pq_heap create / empty guards");
    return 0;
}

static int test_heap_order(void) {
    printf("\n=== test_heap_order ===\n");

    pq_heap_t *h = pq_heap_create();

    /* Insert in reverse order */
    uint64_t keys[] = { 50, 10, 30, 5, 20 };
    for (int i = 0; i < 5; i++) {
        pq_entry_t e = { keys[i], NULL, (uint32_t)i };
        TEST_ASSERT(pq_heap_push(h, &e) == 0, "push ok");
    }
    TEST_ASSERT(pq_heap_count(h) == 5, "count = 5");

    /* Peek should be min = 5 */
    pq_entry_t top;
    TEST_ASSERT(pq_heap_peek(h, &top) == 0, "peek ok");
    TEST_ASSERT(top.key == 5, "peek is min");

    /* Pop in ascending order */
    uint64_t expected[] = { 5, 10, 20, 30, 50 };
    for (int i = 0; i < 5; i++) {
        pq_entry_t e;
        TEST_ASSERT(pq_heap_pop(h, &e) == 0, "pop ok");
        TEST_ASSERT(e.key == expected[i], "ascending order");
    }
    TEST_ASSERT(pq_heap_count(h) == 0, "empty after all pops");

    pq_heap_destroy(h);
    TEST_PASS("pq_heap ascending order");
    return 0;
}

static int test_heap_clear(void) {
    printf("\n=== test_heap_clear ===\n");

    pq_heap_t *h = pq_heap_create();
    for (int i = 0; i < 8; i++) {
        pq_entry_t e = { (uint64_t)i, NULL, (uint32_t)i };
        pq_heap_push(h, &e);
    }
    TEST_ASSERT(pq_heap_count(h) == 8, "8 entries before clear");
    pq_heap_clear(h);
    TEST_ASSERT(pq_heap_count(h) == 0, "0 after clear");

    pq_heap_destroy(h);
    TEST_PASS("pq_heap clear");
    return 0;
}

static int test_heap_full(void) {
    printf("\n=== test_heap_full ===\n");

    pq_heap_t *h = pq_heap_create();
    for (int i = 0; i < PQ_MAX_SIZE; i++) {
        pq_entry_t e = { (uint64_t)i, NULL, (uint32_t)i };
        TEST_ASSERT(pq_heap_push(h, &e) == 0, "push within cap");
    }
    /* One more should fail */
    pq_entry_t extra = { 999, NULL, 999 };
    TEST_ASSERT(pq_heap_push(h, &extra) == -1, "full → -1");

    pq_heap_destroy(h);
    TEST_PASS("pq_heap capacity enforcement");
    return 0;
}

/* ── pq_stats ────────────────────────────────────────────────────── */

static int test_pq_stats(void) {
    printf("\n=== test_pq_stats ===\n");

    pq_stats_t *st = pq_stats_create();
    TEST_ASSERT(st != NULL, "created");

    pq_stats_record_push(st, 1);
    pq_stats_record_push(st, 5);  /* peak = 5 */
    pq_stats_record_push(st, 3);
    pq_stats_record_pop(st);
    pq_stats_record_pop(st);
    pq_stats_record_overflow(st);
    pq_stats_record_overflow(st);

    pq_stats_snapshot_t snap;
    TEST_ASSERT(pq_stats_snapshot(st, &snap) == 0, "snapshot ok");
    TEST_ASSERT(snap.push_count     == 3, "3 pushes");
    TEST_ASSERT(snap.pop_count      == 2, "2 pops");
    TEST_ASSERT(snap.peak_size      == 5, "peak = 5");
    TEST_ASSERT(snap.overflow_count == 2, "2 overflows");

    pq_stats_reset(st);
    pq_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.push_count == 0, "reset ok");

    pq_stats_destroy(st);
    TEST_PASS("pq_stats push/pop/overflow/peak/snapshot/reset");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_heap_create();
    failures += test_heap_order();
    failures += test_heap_clear();
    failures += test_heap_full();
    failures += test_pq_stats();

    printf("\n");
    if (failures == 0) printf("ALL PQUEUE TESTS PASSED\n");
    else               printf("%d PQUEUE TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
