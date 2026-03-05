/*
 * test_sigroute.c — Unit tests for PHASE-81 Signal Router
 *
 * Tests sr_signal (init/null), sr_route (add/route/remove/mask-matching/
 * filter/cap/wildcard), and sr_stats (record_route/snapshot/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../src/sigroute/sr_signal.h"
#include "../../src/sigroute/sr_route.h"
#include "../../src/sigroute/sr_stats.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── sr_signal ───────────────────────────────────────────────────── */

static int test_signal_init(void) {
    printf("\n=== test_signal_init ===\n");

    sr_signal_t s;
    TEST_ASSERT(sr_signal_init(&s, 10, 5, 99, 12345) == 0, "init ok");
    TEST_ASSERT(s.signal_id    == 10,    "signal_id");
    TEST_ASSERT(s.level        ==  5,    "level");
    TEST_ASSERT(s.source_id    == 99,    "source_id");
    TEST_ASSERT(s.timestamp_us == 12345, "timestamp_us");

    TEST_ASSERT(sr_signal_init(NULL, 1, 0, 0, 0) == -1, "NULL → -1");

    TEST_PASS("sr_signal init / null guard");
    return 0;
}

/* ── sr_route ────────────────────────────────────────────────────── */

static int g_delivered = 0;
static void on_deliver(const sr_signal_t *s, void *user) { (void)s; (void)user; g_delivered++; }

static int test_route_basic(void) {
    printf("\n=== test_route_basic ===\n");

    sr_router_t *r = sr_router_create();
    TEST_ASSERT(r != NULL, "created");
    TEST_ASSERT(sr_router_count(r) == 0, "initially 0");

    /* Route matches signal_id & 0xFFFF == 10 */
    sr_route_handle_t h = sr_router_add_route(r, 0xFFFF, 10, NULL, on_deliver, NULL);
    TEST_ASSERT(h != SR_INVALID_HANDLE, "route added");
    TEST_ASSERT(sr_router_count(r) == 1, "count = 1");

    sr_signal_t s;
    sr_signal_init(&s, 10, 1, 0, 0);

    g_delivered = 0;
    int n = sr_router_route(r, &s);
    TEST_ASSERT(n == 1, "1 delivery");
    TEST_ASSERT(g_delivered == 1, "callback called");

    /* Non-matching signal */
    sr_signal_init(&s, 20, 1, 0, 0);
    g_delivered = 0;
    n = sr_router_route(r, &s);
    TEST_ASSERT(n == 0, "no match → 0");
    TEST_ASSERT(g_delivered == 0, "no callback");

    /* Remove route */
    TEST_ASSERT(sr_router_remove_route(r, h) == 0, "remove ok");
    TEST_ASSERT(sr_router_count(r) == 0, "count = 0");
    TEST_ASSERT(sr_router_remove_route(r, h) == -1, "double-remove → -1");

    sr_router_destroy(r);
    TEST_PASS("sr_route add/route/remove/no-match");
    return 0;
}

static bool filter_high_level(const sr_signal_t *s, void *user) {
    (void)user; return s->level >= 10;
}

static int test_route_filter(void) {
    printf("\n=== test_route_filter ===\n");

    sr_router_t *r = sr_router_create();
    /* Wildcard mask: all signals reach this route, then filter by level */
    sr_router_add_route(r, 0, 0, filter_high_level, on_deliver, NULL);

    sr_signal_t s;
    sr_signal_init(&s, 42, 5, 0, 0);   /* level 5 < 10 → filtered */
    g_delivered = 0;
    TEST_ASSERT(sr_router_route(r, &s) == 0, "low level → filtered out");
    TEST_ASSERT(g_delivered == 0, "no callback");

    sr_signal_init(&s, 42, 15, 0, 0);  /* level 15 ≥ 10 → passes */
    g_delivered = 0;
    TEST_ASSERT(sr_router_route(r, &s) == 1, "high level → delivered");
    TEST_ASSERT(g_delivered == 1, "callback called");

    sr_router_destroy(r);
    TEST_PASS("sr_route filter predicate");
    return 0;
}

/* ── sr_stats ────────────────────────────────────────────────────── */

static int test_sr_stats(void) {
    printf("\n=== test_sr_stats ===\n");

    sr_stats_t *st = sr_stats_create();
    TEST_ASSERT(st != NULL, "created");

    sr_stats_record_route(st, 1, 0);  /* delivered */
    sr_stats_record_route(st, 2, 1);  /* delivered + 1 filtered */
    sr_stats_record_route(st, 0, 2);  /* 0 delivered, 2 filtered */
    sr_stats_record_route(st, 0, 0);  /* dropped */

    sr_stats_snapshot_t snap;
    TEST_ASSERT(sr_stats_snapshot(st, &snap) == 0, "snapshot ok");
    TEST_ASSERT(snap.routed   == 2, "2 routed events");
    TEST_ASSERT(snap.filtered == 3, "3 filtered callbacks");
    TEST_ASSERT(snap.dropped  == 1, "1 dropped");

    sr_stats_reset(st);
    sr_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.routed == 0, "reset ok");

    sr_stats_destroy(st);
    TEST_PASS("sr_stats routed/filtered/dropped/snapshot/reset");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_signal_init();
    failures += test_route_basic();
    failures += test_route_filter();
    failures += test_sr_stats();

    printf("\n");
    if (failures == 0) printf("ALL SIGROUTE TESTS PASSED\n");
    else               printf("%d SIGROUTE TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
