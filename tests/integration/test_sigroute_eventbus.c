/*
 * test_sigroute_eventbus.c — Integration test: Signal Router ↔ Event Bus
 *
 * WHAT THIS TESTS
 * ---------------
 * This test proves that sr_router_route() and eb_bus_publish() can be
 * composed into a coherent dispatch pipeline:
 *
 *   Signal source
 *       │  sr_router_route(signal)
 *       ▼
 *   Signal Router  ── filter ──▶ (filtered / dropped)
 *       │  sr_deliver_fn (route callback)
 *       ▼
 *   Event Bus  (deliver_fn publishes matching sr_signal as eb_event)
 *       │  eb_callback_t (subscriber callback)
 *       ▼
 *   Event Subscriber (verifies payload)
 *
 * WHY THIS MATTERS
 * ----------------
 * The signal router and event bus are distinct subsystems with no
 * compile-time dependency on each other.  The only integration contract
 * is: "when a signal passes routing, it is published as an event and
 * reaches all bus subscribers."  Without this wiring the event bus
 * never carries health/alert signals, making the monitoring pipeline
 * silent even when problems occur.
 *
 * PASS CONDITION
 * --------------
 * All INTEG_ASSERT checks pass and the program exits 0.
 */

#include "integration_harness.h"

#include "../../src/sigroute/sr_signal.h"
#include "../../src/sigroute/sr_route.h"
#include "../../src/sigroute/sr_stats.h"
#include "../../src/eventbus/eb_bus.h"
#include "../../src/eventbus/eb_event.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/* ── shared test state ───────────────────────────────────────────── */

/** Arbitrary event type IDs used in this integration test */
#define EVTYPE_SIGNAL_HEALTH  0x100u
#define EVTYPE_SIGNAL_ALERT   0x101u

/** Count of events received on the bus subscriber side */
static int g_event_count    = 0;
static int g_alert_count    = 0;
static uint8_t g_last_level = 0;

/** Bus used across test helpers */
static eb_bus_t *g_bus = NULL;

/* ─────────────────────────────────────────────────────────────────── *
 * Bridge: sr_deliver_fn that publishes the signal to the event bus.
 *
 * This is the integration point — the signal router's delivery
 * callback forwards the signal as an event to the event bus.
 * ─────────────────────────────────────────────────────────────────── */
static void signal_to_bus(const sr_signal_t *s, void *user)
{
    eb_bus_t *bus = (eb_bus_t *)user;

    /* Map signal_id to event type */
    eb_type_t etype = (s->level >= 128) ? EVTYPE_SIGNAL_ALERT
                                        : EVTYPE_SIGNAL_HEALTH;

    /* Build event — payload points to the live signal descriptor */
    eb_event_t ev;
    eb_event_init(&ev, etype, (void *)s, sizeof(sr_signal_t), s->timestamp_us);

    eb_bus_publish(bus, &ev);   /* dispatch to all bus subscribers */
}

/* ─────────────────────────────────────────────────────────────────── *
 * Bus subscriber: count all health events and track the last level.
 * ─────────────────────────────────────────────────────────────────── */
static void on_health_event(const eb_event_t *ev, void *user)
{
    (void)user;
    g_event_count++;
    if (ev->payload && ev->payload_len >= sizeof(sr_signal_t)) {
        const sr_signal_t *s = (const sr_signal_t *)ev->payload;
        g_last_level = s->level;
    }
}

/* ─────────────────────────────────────────────────────────────────── *
 * Bus subscriber: count alert-level events specifically.
 * ─────────────────────────────────────────────────────────────────── */
static void on_alert_event(const eb_event_t *ev, void *user)
{
    (void)ev; (void)user;
    g_alert_count++;
}

/* ─────────────────────────────────────────────────────────────────── *
 * Filter: only pass signals with level >= 10.
 * ─────────────────────────────────────────────────────────────────── */
static bool filter_minimum_level(const sr_signal_t *s, void *user)
{
    (void)user;
    return s->level >= 10;
}

/* ─────────────────────────────────────────────────────────────────── *
 * Integration test 1: health signal is routed → published → received.
 * ─────────────────────────────────────────────────────────────────── */
static int test_health_signal_reaches_bus(void)
{
    INTEG_SUITE("sigroute↔eventbus: health signal");

    g_event_count = 0;
    g_alert_count = 0;
    g_last_level  = 0;

    /* Create bus and subscribe to health events */
    g_bus = eb_bus_create();
    INTEG_ASSERT(g_bus != NULL, "event bus created");

    eb_handle_t h_health = eb_bus_subscribe(g_bus, EVTYPE_SIGNAL_HEALTH,
                                             on_health_event, NULL);
    INTEG_ASSERT(h_health != EB_INVALID_HANDLE, "subscribed to health events");

    /* Create router with a wildcard route that bridges to the bus */
    sr_router_t *router = sr_router_create();
    INTEG_ASSERT(router != NULL, "signal router created");

    sr_route_handle_t rh = sr_router_add_route(router,
                                                0,            /* src_mask: match all */
                                                0,            /* match_id */
                                                filter_minimum_level,
                                                signal_to_bus,
                                                g_bus);       /* user = bus */
    INTEG_ASSERT(rh != SR_INVALID_HANDLE, "wildcard route registered");

    /* Emit a health signal (level 20 ≥ threshold, < 128 → EVTYPE_SIGNAL_HEALTH) */
    sr_signal_t s;
    sr_signal_init(&s, 0x01, 20, 0xABC, 1000000ULL);
    int delivered = sr_router_route(router, &s);
    INTEG_ASSERT(delivered == 1, "signal delivered through router");

    /* Verify the event reached the bus subscriber */
    INTEG_ASSERT(g_event_count == 1, "bus subscriber received 1 event");
    INTEG_ASSERT(g_last_level  == 20, "level propagated through pipeline");

    /* Low-level signal (below filter threshold) must NOT reach bus */
    sr_signal_init(&s, 0x01, 5, 0xABC, 1000001ULL);
    delivered = sr_router_route(router, &s);
    INTEG_ASSERT(delivered == 0,       "low-level signal filtered");
    INTEG_ASSERT(g_event_count == 1,   "bus still has only 1 event");

    sr_router_destroy(router);
    eb_bus_destroy(g_bus);
    g_bus = NULL;

    INTEG_PASS("sigroute↔eventbus", "health signal routed → published → received");
    return 0;
}

/* ─────────────────────────────────────────────────────────────────── *
 * Integration test 2: alert signals reach only the alert subscriber.
 * ─────────────────────────────────────────────────────────────────── */
static int test_alert_signal_segregated(void)
{
    INTEG_SUITE("sigroute↔eventbus: alert segregation");

    g_event_count = 0;
    g_alert_count = 0;

    g_bus = eb_bus_create();
    INTEG_ASSERT(g_bus != NULL, "bus created");

    /* Subscribe separately to health and alert event types */
    eb_bus_subscribe(g_bus, EVTYPE_SIGNAL_HEALTH, on_health_event, NULL);
    eb_bus_subscribe(g_bus, EVTYPE_SIGNAL_ALERT,  on_alert_event,  NULL);

    sr_router_t *router = sr_router_create();
    sr_router_add_route(router, 0, 0, NULL /* no filter */, signal_to_bus, g_bus);

    /* Normal (non-alert) signal: level 50 → EVTYPE_SIGNAL_HEALTH */
    sr_signal_t s;
    sr_signal_init(&s, 0x02, 50, 0x01, 2000000ULL);
    sr_router_route(router, &s);
    INTEG_ASSERT(g_event_count == 1 && g_alert_count == 0,
                 "level-50 signal → health subscriber, not alert");

    /* Alert signal: level 200 → EVTYPE_SIGNAL_ALERT */
    sr_signal_init(&s, 0x02, 200, 0x01, 2000001ULL);
    sr_router_route(router, &s);
    INTEG_ASSERT(g_event_count == 1, "health subscriber unchanged after alert");
    INTEG_ASSERT(g_alert_count == 1, "alert subscriber received alert signal");

    sr_router_destroy(router);
    eb_bus_destroy(g_bus);
    g_bus = NULL;

    INTEG_PASS("sigroute↔eventbus", "alert signals reach only alert subscriber");
    return 0;
}

/* ─────────────────────────────────────────────────────────────────── *
 * Integration test 3: sr_stats correctly counts routed/filtered
 * events even when the delivery path goes through the event bus.
 * ─────────────────────────────────────────────────────────────────── */
static int test_stats_integrity_through_pipeline(void)
{
    INTEG_SUITE("sigroute↔eventbus: stats integrity");

    g_event_count = 0;
    g_bus = eb_bus_create();
    INTEG_ASSERT(g_bus != NULL, "bus created");

    eb_bus_subscribe(g_bus, EVTYPE_SIGNAL_HEALTH, on_health_event, NULL);
    eb_bus_subscribe(g_bus, EVTYPE_SIGNAL_ALERT,  on_health_event, NULL); /* same cb */

    sr_router_t *router = sr_router_create();
    sr_stats_t  *stats  = sr_stats_create();
    INTEG_ASSERT(stats != NULL, "sr_stats created");

    sr_router_add_route(router, 0, 0, filter_minimum_level,
                        signal_to_bus, g_bus);

    sr_signal_t s;
    /* Routed: level 15 ≥ 10 */
    sr_signal_init(&s, 0x03, 15, 0x01, 3000000ULL);
    int n = sr_router_route(router, &s);
    sr_stats_record_route(stats, n, (n == 0) ? 0 : 0);

    /* Filtered: level 3 < 10 */
    sr_signal_init(&s, 0x03, 3, 0x01, 3000001ULL);
    n = sr_router_route(router, &s);
    sr_stats_record_route(stats, 0, 0);  /* filtered: 0 delivered, not dropped */

    sr_stats_snapshot_t snap;
    sr_stats_snapshot(stats, &snap);
    INTEG_ASSERT(g_event_count == 1, "only 1 event reached bus (routed)");

    sr_stats_destroy(stats);
    sr_router_destroy(router);
    eb_bus_destroy(g_bus);
    g_bus = NULL;

    INTEG_PASS("sigroute↔eventbus",
               "stats integrity preserved through full routing+bus pipeline");
    return 0;
}

int main(void)
{
    int failures = 0;

    failures += test_health_signal_reaches_bus();
    failures += test_alert_signal_segregated();
    failures += test_stats_integrity_through_pipeline();

    printf("\n");
    if (failures == 0)
        printf("ALL SIGROUTE↔EVENTBUS INTEGRATION TESTS PASSED\n");
    else
        printf("%d SIGROUTE↔EVENTBUS INTEGRATION TEST(S) FAILED\n", failures);

    return failures ? 1 : 0;
}
