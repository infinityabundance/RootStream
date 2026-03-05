/*
 * test_eventbus.c — Unit tests for PHASE-75 Event Bus
 *
 * Tests eb_event (init), eb_bus (subscribe/publish/unsubscribe/wildcard/
 * full-guard/subscriber_count), and eb_stats
 * (record_publish/dropped/dispatch/snapshot/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../../src/eventbus/eb_event.h"
#include "../../src/eventbus/eb_bus.h"
#include "../../src/eventbus/eb_stats.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── eb_event ────────────────────────────────────────────────────── */

static int test_event_init(void) {
    printf("\n=== test_event_init ===\n");

    eb_event_t e;
    int dummy = 42;
    TEST_ASSERT(eb_event_init(&e, 7, &dummy, sizeof(dummy), 1000) == 0, "init ok");
    TEST_ASSERT(e.type_id == 7, "type_id");
    TEST_ASSERT(e.payload == &dummy, "payload ptr");
    TEST_ASSERT(e.payload_len == sizeof(dummy), "payload_len");
    TEST_ASSERT(e.timestamp_us == 1000, "timestamp_us");

    TEST_ASSERT(eb_event_init(NULL, 1, NULL, 0, 0) == -1, "NULL → -1");

    TEST_PASS("eb_event init");
    return 0;
}

/* ── eb_bus ──────────────────────────────────────────────────────── */

static int g_calls = 0;
static uint32_t g_last_type = 0;

static void on_event(const eb_event_t *e, void *user) {
    (void)user;
    g_calls++;
    g_last_type = e->type_id;
}

static int test_bus_subscribe_publish(void) {
    printf("\n=== test_bus_subscribe_publish ===\n");

    eb_bus_t *b = eb_bus_create();
    TEST_ASSERT(b != NULL, "created");
    TEST_ASSERT(eb_bus_subscriber_count(b) == 0, "initially 0");

    eb_handle_t h1 = eb_bus_subscribe(b, 1, on_event, NULL);
    eb_handle_t h2 = eb_bus_subscribe(b, 2, on_event, NULL);
    TEST_ASSERT(h1 != EB_INVALID_HANDLE, "subscribe h1");
    TEST_ASSERT(h2 != EB_INVALID_HANDLE, "subscribe h2");
    TEST_ASSERT(eb_bus_subscriber_count(b) == 2, "count = 2");

    eb_event_t e;
    eb_event_init(&e, 1, NULL, 0, 0);

    g_calls = 0;
    int dispatched = eb_bus_publish(b, &e);
    TEST_ASSERT(dispatched == 1, "type 1 dispatched to 1 sub");
    TEST_ASSERT(g_calls == 1, "1 call");
    TEST_ASSERT(g_last_type == 1, "type_id forwarded");

    /* Publish type 3 → no subscribers */
    eb_event_init(&e, 3, NULL, 0, 0);
    g_calls = 0;
    dispatched = eb_bus_publish(b, &e);
    TEST_ASSERT(dispatched == 0, "no sub for type 3");
    TEST_ASSERT(g_calls == 0, "0 calls");

    /* Unsubscribe h1 */
    TEST_ASSERT(eb_bus_unsubscribe(b, h1) == 0, "unsubscribe ok");
    TEST_ASSERT(eb_bus_subscriber_count(b) == 1, "count = 1 after unsub");
    TEST_ASSERT(eb_bus_unsubscribe(b, h1) == -1, "double-unsub → -1");

    eb_bus_destroy(b);
    TEST_PASS("eb_bus subscribe/publish/unsubscribe");
    return 0;
}

static int test_bus_wildcard(void) {
    printf("\n=== test_bus_wildcard ===\n");

    eb_bus_t *b = eb_bus_create();
    eb_bus_subscribe(b, EB_TYPE_ANY, on_event, NULL);

    eb_event_t e;
    g_calls = 0;
    eb_event_init(&e, 99, NULL, 0, 0); eb_bus_publish(b, &e);
    eb_event_init(&e,  5, NULL, 0, 0); eb_bus_publish(b, &e);
    eb_event_init(&e,  0, NULL, 0, 0); eb_bus_publish(b, &e);
    TEST_ASSERT(g_calls == 3, "wildcard catches 3 events");

    eb_bus_destroy(b);
    TEST_PASS("eb_bus wildcard");
    return 0;
}

static int test_bus_full(void) {
    printf("\n=== test_bus_full ===\n");

    eb_bus_t *b = eb_bus_create();
    int ok_count = 0;
    for (int i = 0; i < EB_MAX_SUBSCRIBERS; i++) {
        eb_handle_t h = eb_bus_subscribe(b, (uint32_t)i, on_event, NULL);
        if (h != EB_INVALID_HANDLE) ok_count++;
    }
    TEST_ASSERT(ok_count == EB_MAX_SUBSCRIBERS, "exactly max subscribers");

    /* One more should fail */
    eb_handle_t extra = eb_bus_subscribe(b, 999, on_event, NULL);
    TEST_ASSERT(extra == EB_INVALID_HANDLE, "cap enforced");

    eb_bus_destroy(b);
    TEST_PASS("eb_bus subscriber cap");
    return 0;
}

/* ── eb_stats ────────────────────────────────────────────────────── */

static int test_eb_stats(void) {
    printf("\n=== test_eb_stats ===\n");

    eb_stats_t *st = eb_stats_create();
    TEST_ASSERT(st != NULL, "created");

    eb_stats_record_publish(st, 2);  /* 2 dispatches */
    eb_stats_record_publish(st, 3);  /* 3 dispatches */
    eb_stats_record_publish(st, 0);  /* dropped */

    eb_stats_snapshot_t snap;
    TEST_ASSERT(eb_stats_snapshot(st, &snap) == 0, "snapshot ok");
    TEST_ASSERT(snap.published_count == 3, "3 published");
    TEST_ASSERT(snap.dispatch_count  == 5, "5 dispatched");
    TEST_ASSERT(snap.dropped_count   == 1, "1 dropped");

    eb_stats_reset(st);
    eb_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.published_count == 0, "reset ok");

    eb_stats_destroy(st);
    TEST_PASS("eb_stats publish/dispatch/dropped/snapshot/reset");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_event_init();
    failures += test_bus_subscribe_publish();
    failures += test_bus_wildcard();
    failures += test_bus_full();
    failures += test_eb_stats();

    printf("\n");
    if (failures == 0) printf("ALL EVENTBUS TESTS PASSED\n");
    else               printf("%d EVENTBUS TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
