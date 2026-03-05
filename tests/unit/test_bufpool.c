/*
 * test_bufpool.c — Unit tests for PHASE-74 Buffer Pool
 *
 * Tests bp_pool (create/acquire/release/peak/capacity/exhaust),
 * and bp_stats (alloc/free/fail/peak/snapshot/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../../src/bufpool/bp_block.h"
#include "../../src/bufpool/bp_pool.h"
#include "../../src/bufpool/bp_stats.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── bp_pool ─────────────────────────────────────────────────────── */

static int test_pool_create(void) {
    printf("\n=== test_pool_create ===\n");

    bp_pool_t *p = bp_pool_create(4, 1024);
    TEST_ASSERT(p != NULL, "created");
    TEST_ASSERT(bp_pool_capacity(p) == 4, "capacity = 4");
    TEST_ASSERT(bp_pool_in_use(p)   == 0, "in_use = 0");
    TEST_ASSERT(bp_pool_peak(p)     == 0, "peak = 0");
    bp_pool_destroy(p);

    /* Invalid params */
    TEST_ASSERT(bp_pool_create(0, 1024) == NULL, "n=0 → NULL");
    TEST_ASSERT(bp_pool_create(4, 0)    == NULL, "size=0 → NULL");
    TEST_ASSERT(bp_pool_create(BP_MAX_BLOCKS + 1, 1) == NULL, "n>max → NULL");

    TEST_PASS("bp_pool create / invalid params");
    return 0;
}

static int test_pool_acquire_release(void) {
    printf("\n=== test_pool_acquire_release ===\n");

    bp_pool_t *p = bp_pool_create(3, 64);
    TEST_ASSERT(p != NULL, "created");

    bp_block_t *b1 = bp_pool_acquire(p);
    bp_block_t *b2 = bp_pool_acquire(p);
    bp_block_t *b3 = bp_pool_acquire(p);

    TEST_ASSERT(b1 != NULL, "acquire b1");
    TEST_ASSERT(b2 != NULL, "acquire b2");
    TEST_ASSERT(b3 != NULL, "acquire b3");
    TEST_ASSERT(b1 != b2 && b2 != b3, "distinct blocks");
    TEST_ASSERT(b1->size == 64, "block size");
    TEST_ASSERT(bp_pool_in_use(p) == 3, "all 3 in use");
    TEST_ASSERT(bp_pool_peak(p)   == 3, "peak = 3");

    /* Pool exhausted → NULL */
    TEST_ASSERT(bp_pool_acquire(p) == NULL, "exhaust → NULL");

    /* Release one and re-acquire */
    TEST_ASSERT(bp_pool_release(p, b2) == 0, "release b2 ok");
    TEST_ASSERT(bp_pool_in_use(p) == 2, "in_use = 2 after release");

    bp_block_t *b4 = bp_pool_acquire(p);
    TEST_ASSERT(b4 != NULL, "re-acquire after release");

    /* Write to block data to verify it is usable */
    memset(b1->data, 0xAB, b1->size);
    TEST_ASSERT(((uint8_t *)b1->data)[0] == 0xAB, "data writeable");

    /* Release all */
    bp_pool_release(p, b1);
    bp_pool_release(p, b3);
    bp_pool_release(p, b4);
    TEST_ASSERT(bp_pool_in_use(p) == 0, "all released");

    /* Double-release should fail */
    TEST_ASSERT(bp_pool_release(p, b1) == -1, "double-release → -1");

    bp_pool_destroy(p);
    TEST_PASS("bp_pool acquire/release/exhaust/peak/data");
    return 0;
}

/* ── bp_stats ────────────────────────────────────────────────────── */

static int test_bp_stats(void) {
    printf("\n=== test_bp_stats ===\n");

    bp_stats_t *st = bp_stats_create();
    TEST_ASSERT(st != NULL, "created");

    bp_stats_record_alloc(st, 1);
    bp_stats_record_alloc(st, 3);   /* peak = 3 */
    bp_stats_record_alloc(st, 2);
    bp_stats_record_free(st);
    bp_stats_record_free(st);
    bp_stats_record_fail(st);
    bp_stats_record_fail(st);

    bp_stats_snapshot_t snap;
    TEST_ASSERT(bp_stats_snapshot(st, &snap) == 0, "snapshot ok");
    TEST_ASSERT(snap.alloc_count == 3, "3 allocs");
    TEST_ASSERT(snap.free_count  == 2, "2 frees");
    TEST_ASSERT(snap.peak_in_use == 3, "peak = 3");
    TEST_ASSERT(snap.fail_count  == 2, "2 fails");

    bp_stats_reset(st);
    bp_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.alloc_count == 0, "reset ok");

    bp_stats_destroy(st);
    TEST_PASS("bp_stats alloc/free/fail/peak/snapshot/reset");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_pool_create();
    failures += test_pool_acquire_release();
    failures += test_bp_stats();

    printf("\n");
    if (failures == 0) printf("ALL BUFPOOL TESTS PASSED\n");
    else               printf("%d BUFPOOL TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
