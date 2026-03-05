/*
 * test_session_limit.c — Unit tests for PHASE-72 Session Limiter
 *
 * Tests sl_entry (init/state_name), sl_table (create/add/remove/get/
 * cap-enforcement/foreach/count), and sl_stats
 * (admit/reject/eviction/peak/snapshot/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../../src/session_limit/sl_entry.h"
#include "../../src/session_limit/sl_table.h"
#include "../../src/session_limit/sl_stats.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── sl_entry ────────────────────────────────────────────────────── */

static int test_entry_init(void) {
    printf("\n=== test_entry_init ===\n");

    sl_entry_t e;
    TEST_ASSERT(sl_entry_init(&e, 42, "192.168.1.1", 1000) == 0, "init ok");
    TEST_ASSERT(e.session_id == 42, "session_id");
    TEST_ASSERT(strcmp(e.remote_ip, "192.168.1.1") == 0, "remote_ip");
    TEST_ASSERT(e.start_us == 1000, "start_us");
    TEST_ASSERT(e.state == SL_CONNECTING, "initially CONNECTING");
    TEST_ASSERT(e.in_use, "in_use");

    TEST_ASSERT(sl_entry_init(NULL, 1, "x", 0) == -1, "NULL → -1");

    TEST_ASSERT(strcmp(sl_state_name(SL_CONNECTING), "CONNECTING") == 0, "CONNECTING");
    TEST_ASSERT(strcmp(sl_state_name(SL_ACTIVE),     "ACTIVE")     == 0, "ACTIVE");
    TEST_ASSERT(strcmp(sl_state_name(SL_CLOSING),    "CLOSING")    == 0, "CLOSING");

    TEST_PASS("sl_entry init / state names");
    return 0;
}

/* ── sl_table ────────────────────────────────────────────────────── */

static int test_table_add_remove(void) {
    printf("\n=== test_table_add_remove ===\n");

    sl_table_t *t = sl_table_create(3);
    TEST_ASSERT(t != NULL, "created");
    TEST_ASSERT(sl_table_count(t) == 0, "initially 0");

    sl_entry_t *e1 = sl_table_add(t, 1, "10.0.0.1", 1000);
    sl_entry_t *e2 = sl_table_add(t, 2, "10.0.0.2", 2000);
    TEST_ASSERT(e1 != NULL, "add e1 ok");
    TEST_ASSERT(e2 != NULL, "add e2 ok");
    TEST_ASSERT(sl_table_count(t) == 2, "count = 2");

    /* Get */
    TEST_ASSERT(sl_table_get(t, 1) == e1, "get e1");
    TEST_ASSERT(sl_table_get(t, 99) == NULL, "unknown → NULL");

    /* Remove */
    TEST_ASSERT(sl_table_remove(t, 1) == 0, "remove ok");
    TEST_ASSERT(sl_table_count(t) == 1, "count = 1 after remove");
    TEST_ASSERT(sl_table_remove(t, 1) == -1, "remove missing → -1");

    sl_table_destroy(t);
    TEST_PASS("sl_table add/remove/get/count");
    return 0;
}

static int test_table_cap(void) {
    printf("\n=== test_table_cap ===\n");

    sl_table_t *t = sl_table_create(2);

    sl_table_add(t, 1, "a", 0);
    sl_table_add(t, 2, "b", 0);
    /* Third add should fail (cap=2) */
    sl_entry_t *over = sl_table_add(t, 3, "c", 0);
    TEST_ASSERT(over == NULL, "cap enforcement: 3rd add → NULL");

    sl_table_destroy(t);
    TEST_PASS("sl_table cap enforcement");
    return 0;
}

static void count_cb(sl_entry_t *e, void *user) {
    (void)e;
    (*(int *)user)++;
}

static int test_table_foreach(void) {
    printf("\n=== test_table_foreach ===\n");

    sl_table_t *t = sl_table_create(10);
    sl_table_add(t, 1, "a", 0);
    sl_table_add(t, 2, "b", 0);
    sl_table_add(t, 3, "c", 0);

    int count = 0;
    sl_table_foreach(t, count_cb, &count);
    TEST_ASSERT(count == 3, "foreach visits 3 entries");

    sl_table_destroy(t);
    TEST_PASS("sl_table foreach");
    return 0;
}

/* ── sl_stats ────────────────────────────────────────────────────── */

static int test_sl_stats(void) {
    printf("\n=== test_sl_stats ===\n");

    sl_stats_t *st = sl_stats_create();
    TEST_ASSERT(st != NULL, "created");

    sl_stats_record_admit(st, 1);
    sl_stats_record_admit(st, 2);
    sl_stats_record_admit(st, 5);   /* peak = 5 */
    sl_stats_record_reject(st);
    sl_stats_record_reject(st);
    sl_stats_record_eviction(st);

    sl_stats_snapshot_t snap;
    TEST_ASSERT(sl_stats_snapshot(st, &snap) == 0, "snapshot ok");
    TEST_ASSERT(snap.total_admitted  == 3, "3 admitted");
    TEST_ASSERT(snap.total_rejected  == 2, "2 rejected");
    TEST_ASSERT(snap.peak_count      == 5, "peak = 5");
    TEST_ASSERT(snap.eviction_count  == 1, "1 eviction");

    sl_stats_reset(st);
    sl_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.total_admitted == 0, "reset ok");

    sl_stats_destroy(st);
    TEST_PASS("sl_stats admit/reject/evict/peak/snapshot/reset");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_entry_init();
    failures += test_table_add_remove();
    failures += test_table_cap();
    failures += test_table_foreach();
    failures += test_sl_stats();

    printf("\n");
    if (failures == 0) printf("ALL SESSION LIMIT TESTS PASSED\n");
    else               printf("%d SESSION LIMIT TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
