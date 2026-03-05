/*
 * test_reorder.c — Unit tests for PHASE-61 Packet Reorder Buffer
 *
 * Tests reorder_slot (fill/clear), reorder_buffer (in-order delivery,
 * out-of-order reordering, timeout flush, duplicate/full guard,
 * set_timeout), and reorder_stats (insert/deliver/discard/snapshot/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/reorder/reorder_slot.h"
#include "../../src/reorder/reorder_buffer.h"
#include "../../src/reorder/reorder_stats.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

/* ── reorder_slot tests ──────────────────────────────────────────── */

static int test_slot_fill_clear(void) {
    printf("\n=== test_slot_fill_clear ===\n");

    reorder_slot_t slot;
    uint8_t data[8] = {1,2,3,4,5,6,7,8};
    int rc = reorder_slot_fill(&slot, 42, 999, data, 8);
    TEST_ASSERT(rc == 0, "fill ok");
    TEST_ASSERT(slot.occupied, "occupied");
    TEST_ASSERT(slot.seq == 42, "seq");
    TEST_ASSERT(slot.arrival_us == 999, "arrival");
    TEST_ASSERT(slot.payload_len == 8, "payload_len");
    TEST_ASSERT(memcmp(slot.payload, data, 8) == 0, "payload data");

    reorder_slot_clear(&slot);
    TEST_ASSERT(!slot.occupied, "cleared");

    /* Too-large payload */
    uint8_t big[REORDER_SLOT_MAX_PAYLOAD + 1];
    TEST_ASSERT(reorder_slot_fill(&slot, 1, 0, big, REORDER_SLOT_MAX_PAYLOAD + 1) == -1,
                "oversized payload → -1");

    TEST_PASS("reorder_slot fill / clear");
    return 0;
}

/* ── delivery accumulator ────────────────────────────────────────── */

typedef struct {
    uint16_t seqs[256];
    int      count;
} delivery_t;

static void on_deliver(const reorder_slot_t *slot, void *user) {
    delivery_t *d = (delivery_t *)user;
    if (d->count < 256)
        d->seqs[d->count++] = slot->seq;
}

/* ── reorder_buffer tests ────────────────────────────────────────── */

static int test_buffer_in_order(void) {
    printf("\n=== test_buffer_in_order ===\n");

    delivery_t d = {0};
    reorder_buffer_t *rb = reorder_buffer_create(80000, on_deliver, &d);
    TEST_ASSERT(rb != NULL, "created");

    /* Insert 0, 1, 2 in order */
    reorder_buffer_insert(rb, 0, 100, NULL, 0);
    reorder_buffer_insert(rb, 1, 200, NULL, 0);
    reorder_buffer_insert(rb, 2, 300, NULL, 0);

    int n = reorder_buffer_flush(rb, 1000);
    TEST_ASSERT(n == 3, "3 delivered");
    TEST_ASSERT(d.count == 3, "callback 3×");
    TEST_ASSERT(d.seqs[0] == 0 && d.seqs[1] == 1 && d.seqs[2] == 2, "correct order");

    reorder_buffer_destroy(rb);
    TEST_PASS("reorder_buffer in-order delivery");
    return 0;
}

static int test_buffer_out_of_order(void) {
    printf("\n=== test_buffer_out_of_order ===\n");

    delivery_t d = {0};
    reorder_buffer_t *rb = reorder_buffer_create(80000, on_deliver, &d);

    /* Insert 2, 0, 1 (out-of-order) */
    reorder_buffer_insert(rb, 2, 300, NULL, 0);
    reorder_buffer_insert(rb, 0, 100, NULL, 0);
    reorder_buffer_insert(rb, 1, 200, NULL, 0);

    int n = reorder_buffer_flush(rb, 1000);
    TEST_ASSERT(n == 3, "3 delivered after reorder");
    TEST_ASSERT(d.count == 3, "3 callbacks");
    TEST_ASSERT(d.seqs[0] == 0 && d.seqs[1] == 1 && d.seqs[2] == 2, "delivered in seq order");

    reorder_buffer_destroy(rb);
    TEST_PASS("reorder_buffer out-of-order reordering");
    return 0;
}

static int test_buffer_timeout_flush(void) {
    printf("\n=== test_buffer_timeout_flush ===\n");

    delivery_t d = {0};
    /* 50ms timeout */
    reorder_buffer_t *rb = reorder_buffer_create(50000, on_deliver, &d);

    /* Insert seq 1 and 2 (seq 0 is missing) */
    reorder_buffer_insert(rb, 1, 1000, NULL, 0);
    reorder_buffer_insert(rb, 2, 2000, NULL, 0);

    /* Flush at t=10000: seq 1 arrived at 1000, 1000+50000=51000 > 10000 → not yet */
    reorder_buffer_flush(rb, 10000);
    TEST_ASSERT(d.count == 0, "no delivery before timeout");

    /* Flush at t=55000: seq 1 arrival(1000)+50000=51000 ≤ 55000 → flush */
    int n = reorder_buffer_flush(rb, 55000);
    TEST_ASSERT(n >= 1, "at least 1 timed-out delivery");
    TEST_ASSERT(d.count >= 1, "callback fired");

    reorder_buffer_destroy(rb);
    TEST_PASS("reorder_buffer timeout flush");
    return 0;
}

static int test_buffer_duplicate_guard(void) {
    printf("\n=== test_buffer_duplicate_guard ===\n");

    delivery_t d = {0};
    reorder_buffer_t *rb = reorder_buffer_create(80000, on_deliver, &d);

    reorder_buffer_insert(rb, 5, 100, NULL, 0);
    /* Same seq again (maps to same slot → occupied) */
    int rc = reorder_buffer_insert(rb, 5, 200, NULL, 0);
    TEST_ASSERT(rc == -1, "duplicate insert → -1");
    TEST_ASSERT(reorder_buffer_count(rb) == 1, "only 1 slot occupied");

    reorder_buffer_destroy(rb);
    TEST_PASS("reorder_buffer duplicate guard");
    return 0;
}

static int test_buffer_set_timeout(void) {
    printf("\n=== test_buffer_set_timeout ===\n");

    delivery_t d = {0};
    reorder_buffer_t *rb = reorder_buffer_create(80000, on_deliver, &d);
    TEST_ASSERT(reorder_buffer_set_timeout(rb, 10000) == 0, "set_timeout ok");
    TEST_ASSERT(reorder_buffer_set_timeout(rb, 0) == -1, "zero timeout → -1");
    reorder_buffer_destroy(rb);
    TEST_PASS("reorder_buffer set_timeout");
    return 0;
}

/* ── reorder_stats tests ─────────────────────────────────────────── */

static int test_reorder_stats(void) {
    printf("\n=== test_reorder_stats ===\n");

    reorder_stats_t *st = reorder_stats_create();
    TEST_ASSERT(st != NULL, "created");

    reorder_stats_record_insert(st, 1, 3);
    reorder_stats_record_insert(st, 1, 5);
    reorder_stats_record_insert(st, 0, 0); /* discard */
    reorder_stats_record_deliver(st, 0);   /* in-order */
    reorder_stats_record_deliver(st, 1);   /* late */

    reorder_stats_snapshot_t snap;
    int rc = reorder_stats_snapshot(st, &snap);
    TEST_ASSERT(rc == 0, "snapshot ok");
    TEST_ASSERT(snap.packets_inserted  == 2, "2 inserted");
    TEST_ASSERT(snap.discards          == 1, "1 discard");
    TEST_ASSERT(snap.packets_delivered == 2, "2 delivered");
    TEST_ASSERT(snap.late_flushes      == 1, "1 late flush");
    TEST_ASSERT(snap.max_depth         == 5, "max depth = 5");

    reorder_stats_reset(st);
    reorder_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.packets_inserted == 0, "reset ok");

    reorder_stats_destroy(st);
    TEST_PASS("reorder_stats insert/deliver/discard/snapshot/reset");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_slot_fill_clear();
    failures += test_buffer_in_order();
    failures += test_buffer_out_of_order();
    failures += test_buffer_timeout_flush();
    failures += test_buffer_duplicate_guard();
    failures += test_buffer_set_timeout();
    failures += test_reorder_stats();

    printf("\n");
    if (failures == 0)
        printf("ALL REORDER TESTS PASSED\n");
    else
        printf("%d REORDER TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
