/*
 * test_retry.c — Unit tests for PHASE-78 Retry Manager
 *
 * Tests rm_entry (init/advance/is_due/backoff), rm_table
 * (create/add/remove/get/tick/auto-evict/cap), and rm_stats
 * (attempt/success/expire/max/snapshot/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../../src/retry_mgr/rm_entry.h"
#include "../../src/retry_mgr/rm_table.h"
#include "../../src/retry_mgr/rm_stats.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── rm_entry ────────────────────────────────────────────────────── */

static int test_entry_init(void) {
    printf("\n=== test_entry_init ===\n");

    rm_entry_t e;
    TEST_ASSERT(rm_entry_init(&e, 42, 1000, 500, 3) == 0, "init ok");
    TEST_ASSERT(e.request_id    == 42,   "request_id");
    TEST_ASSERT(e.max_attempts  == 3,    "max_attempts");
    TEST_ASSERT(e.base_delay_us == 500,  "base_delay");
    TEST_ASSERT(e.next_retry_us == 1500, "fires at now+base_delay");
    TEST_ASSERT(e.in_use, "in_use");

    TEST_ASSERT(rm_entry_init(NULL, 1, 0, 100, 3) == -1, "NULL → -1");
    TEST_ASSERT(rm_entry_init(&e, 1, 0, 100, 0)   == -1, "max_attempts=0 → -1");

    TEST_PASS("rm_entry init / NULL guard");
    return 0;
}

static int test_entry_advance_backoff(void) {
    printf("\n=== test_entry_advance_backoff ===\n");

    rm_entry_t e;
    rm_entry_init(&e, 1, 0, 1000, 4);  /* base 1ms: fires at t=1000 */

    /* Not due before delay */
    TEST_ASSERT(!rm_entry_is_due(&e, 500), "not due at 500µs");
    TEST_ASSERT(rm_entry_is_due(&e, 1000), "due at 1000µs");

    /* First advance at t=1000: attempt_count → 1, delay = 1000µs */
    bool more = rm_entry_advance(&e, 1000);
    TEST_ASSERT(more, "1 attempt, more remain");
    TEST_ASSERT(e.attempt_count == 1, "attempt_count = 1");
    TEST_ASSERT(e.next_retry_us == 2000, "next at 1000+1000=2000");
    TEST_ASSERT(!rm_entry_is_due(&e, 1500), "not due at 1500µs");
    TEST_ASSERT(rm_entry_is_due(&e, 2000), "due at 2000µs");

    /* Second advance: attempt 2, delay = 1000×2 = 2000µs → next 4000 */
    rm_entry_advance(&e, 2000);
    TEST_ASSERT(e.next_retry_us == 4000, "exponential: 2000+2000=4000");

    /* Third advance: attempt 3, delay = 4000µs → next 8000 */
    rm_entry_advance(&e, 4000);
    TEST_ASSERT(e.next_retry_us == 8000, "4000+4000=8000");

    /* Fourth advance: attempt 4 = max → returns false */
    more = rm_entry_advance(&e, 8000);
    TEST_ASSERT(!more, "max attempts → no more");

    TEST_PASS("rm_entry advance / backoff");
    return 0;
}

/* ── rm_table ────────────────────────────────────────────────────── */

static int g_tick_calls = 0;
static void tick_cb(rm_entry_t *e, void *user) { (void)e; (void)user; g_tick_calls++; }

static int test_table_add_remove(void) {
    printf("\n=== test_table_add_remove ===\n");

    rm_table_t *t = rm_table_create();
    TEST_ASSERT(t != NULL, "created");
    TEST_ASSERT(rm_table_count(t) == 0, "initially 0");

    rm_entry_t *e1 = rm_table_add(t, 1, 0, 100, 3);
    rm_entry_t *e2 = rm_table_add(t, 2, 0, 200, 5);
    TEST_ASSERT(e1 != NULL, "add e1");
    TEST_ASSERT(e2 != NULL, "add e2");
    TEST_ASSERT(rm_table_count(t) == 2, "count = 2");

    TEST_ASSERT(rm_table_get(t, 1) == e1, "get e1");
    TEST_ASSERT(rm_table_get(t, 99) == NULL, "unknown → NULL");

    TEST_ASSERT(rm_table_remove(t, 1) == 0, "remove ok");
    TEST_ASSERT(rm_table_count(t) == 1, "count = 1");
    TEST_ASSERT(rm_table_remove(t, 1) == -1, "remove missing → -1");

    rm_table_destroy(t);
    TEST_PASS("rm_table add/remove/get/count");
    return 0;
}

static int test_table_tick(void) {
    printf("\n=== test_table_tick ===\n");

    rm_table_t *t = rm_table_create();

    /* Entry 1: base_delay=0 → fires at t=0; max 1 attempt → auto-evicted after tick */
    rm_table_add(t, 10, 0, 0, 1);
    /* Entry 2: base_delay=2000 → fires at t=2000 */
    rm_table_add(t, 20, 0, 2000, 5);

    /* Tick at t=0: entry 10 is due (next_retry=0), entry 20 not */
    g_tick_calls = 0;
    int n = rm_table_tick(t, 0, tick_cb, NULL);
    TEST_ASSERT(n == 1, "1 entry processed at t=0");
    TEST_ASSERT(g_tick_calls == 1, "cb called once");
    /* Entry 10 had max 1 attempt → advance returns false → auto-evicted */
    TEST_ASSERT(rm_table_count(t) == 1, "entry 10 evicted; entry 20 remains");

    /* Tick at t=2000: entry 20 fires */
    g_tick_calls = 0;
    n = rm_table_tick(t, 2000, tick_cb, NULL);
    TEST_ASSERT(n == 1, "1 entry processed at t=2000");
    TEST_ASSERT(rm_table_count(t) == 1, "entry 20 still active (4 attempts left)");

    rm_table_destroy(t);
    TEST_PASS("rm_table tick / auto-evict");
    return 0;
}

/* ── rm_stats ────────────────────────────────────────────────────── */

static int test_rm_stats(void) {
    printf("\n=== test_rm_stats ===\n");

    rm_stats_t *st = rm_stats_create();
    TEST_ASSERT(st != NULL, "created");

    rm_stats_record_attempt(st, 1);
    rm_stats_record_attempt(st, 3);   /* max = 3 */
    rm_stats_record_attempt(st, 2);
    rm_stats_record_success(st);
    rm_stats_record_success(st);
    rm_stats_record_expire(st);

    rm_stats_snapshot_t snap;
    TEST_ASSERT(rm_stats_snapshot(st, &snap) == 0, "snapshot ok");
    TEST_ASSERT(snap.total_attempts    == 3, "3 attempts");
    TEST_ASSERT(snap.total_succeeded   == 2, "2 succeeded");
    TEST_ASSERT(snap.total_expired     == 1, "1 expired");
    TEST_ASSERT(snap.max_attempts_seen == 3, "max = 3");

    rm_stats_reset(st);
    rm_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.total_attempts == 0, "reset ok");

    rm_stats_destroy(st);
    TEST_PASS("rm_stats attempt/success/expire/max/snapshot/reset");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_entry_init();
    failures += test_entry_advance_backoff();
    failures += test_table_add_remove();
    failures += test_table_tick();
    failures += test_rm_stats();

    printf("\n");
    if (failures == 0) printf("ALL RETRY TESTS PASSED\n");
    else               printf("%d RETRY TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
