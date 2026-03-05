/*
 * test_flowctl_metrics.c — Integration test: Flow Controller ↔ Metrics Exporter
 *
 * WHAT THIS TESTS
 * ---------------
 * This test proves that the fc_engine and mx_gauge/mx_registry subsystems
 * are not "ceremonial" — i.e., that real consume/replenish calls produce
 * observable, correct changes in metric gauges when the caller wires them
 * together correctly.
 *
 * The test simulates a realistic streaming session:
 *   1. Create a flow-controller engine and a metrics registry.
 *   2. Register gauges for "fc_bytes_sent", "fc_bytes_dropped",
 *      "fc_stalls", and "fc_replenish_count".
 *   3. Drive a sequence of consume/replenish calls while updating gauges
 *      after each call (exactly as a real send loop would).
 *   4. Assert that the final gauge values match the expected accounting.
 *
 * WHY THIS MATTERS
 * ----------------
 * Neither fc_engine nor mx_gauge knows about the other.  The only
 * contract being tested here is the *wiring* — the caller's responsibility
 * to update metrics after every flow-control decision.  If the wiring
 * is absent the gauges never move, making dashboards useless.
 *
 * PASS CONDITION
 * --------------
 * All INTEG_ASSERT checks pass and the program exits 0.
 */

#include "integration_harness.h"

#include "../../src/flowctl/fc_engine.h"
#include "../../src/flowctl/fc_params.h"
#include "../../src/flowctl/fc_stats.h"
#include "../../src/metrics/mx_gauge.h"
#include "../../src/metrics/mx_registry.h"
#include "../../src/metrics/mx_snapshot.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/* ─────────────────────────────────────────────────────────────────── *
 * Helper: simulate one "send attempt" and wire the result to gauges.
 * Returns 1 if the send succeeded (credit consumed), 0 if stalled.
 * ─────────────────────────────────────────────────────────────────── */
static int attempt_send(fc_engine_t  *engine,
                        fc_stats_t   *stats,
                        mx_gauge_t   *g_bytes_sent,
                        mx_gauge_t   *g_stalls,
                        uint32_t      bytes)
{
    if (fc_engine_can_send(engine, bytes)) {
        /* Credit available — consume and record */
        fc_engine_consume(engine, bytes);
        fc_stats_record_send(stats, bytes);
        mx_gauge_add(g_bytes_sent, (int64_t)bytes);  /* wire: update gauge */
        return 1;
    } else {
        /* Stalled — record and expose via gauge */
        fc_stats_record_stall(stats);
        mx_gauge_add(g_stalls, 1);  /* wire: increment stall counter */
        return 0;
    }
}

/* ─────────────────────────────────────────────────────────────────── *
 * Helper: replenish credit and wire the event to the replenish gauge.
 * ─────────────────────────────────────────────────────────────────── */
static void do_replenish(fc_engine_t *engine,
                         fc_stats_t  *stats,
                         mx_gauge_t  *g_replenish,
                         uint32_t     bytes)
{
    fc_engine_replenish(engine, bytes);
    fc_stats_record_replenish(stats);
    mx_gauge_add(g_replenish, 1);  /* wire: each replenish event counted */
}

/* ─────────────────────────────────────────────────────────────────── *
 * Integration test 1: normal flow — sends succeed, gauges update.
 * ─────────────────────────────────────────────────────────────────── */
static int test_normal_flow(void)
{
    INTEG_SUITE("flowctl↔metrics: normal flow");

    /* ── setup ── */
    fc_params_t p;
    fc_params_init(&p, 4096, 1024, 4096, 256);
    fc_engine_t *engine = fc_engine_create(&p);
    fc_stats_t  *stats  = fc_stats_create();

    mx_registry_t *reg  = mx_registry_create();
    mx_gauge_t *g_sent      = mx_registry_register(reg, "fc_bytes_sent");
    mx_gauge_t *g_stalls    = mx_registry_register(reg, "fc_stalls");
    mx_gauge_t *g_replenish = mx_registry_register(reg, "fc_replenish_count");

    INTEG_ASSERT(engine   != NULL, "fc_engine created");
    INTEG_ASSERT(stats    != NULL, "fc_stats created");
    INTEG_ASSERT(g_sent   != NULL, "gauge fc_bytes_sent registered");
    INTEG_ASSERT(g_stalls != NULL, "gauge fc_stalls registered");
    INTEG_ASSERT(g_replenish != NULL, "gauge fc_replenish_count registered");

    /* ── exercise ──
     * Send 4 × 200-byte frames (total 800 bytes, within 1024 budget).
     * Then replenish. Then send 2 more frames.
     */
    for (int i = 0; i < 4; i++) {
        int ok = attempt_send(engine, stats, g_sent, g_stalls, 200);
        INTEG_ASSERT(ok == 1, "send 200 bytes succeeds within budget");
    }
    /* After 4×200=800 bytes consumed from 1024-byte budget, 224 bytes remain. */

    do_replenish(engine, stats, g_replenish, 800);  /* +800, capped at window=4096 */

    /* Now credit ≥ 1024, send 2 more frames */
    for (int i = 0; i < 2; i++) {
        int ok = attempt_send(engine, stats, g_sent, g_stalls, 200);
        INTEG_ASSERT(ok == 1, "post-replenish send succeeds");
    }

    /* ── verify gauges match expected accounting ── */
    INTEG_ASSERT(mx_gauge_get(g_sent)      == 1200, "fc_bytes_sent gauge = 1200");
    INTEG_ASSERT(mx_gauge_get(g_stalls)    ==    0, "fc_stalls gauge = 0 (no stalls)");
    INTEG_ASSERT(mx_gauge_get(g_replenish) ==    1, "fc_replenish_count gauge = 1");

    /* ── verify fc_stats matches gauges ── */
    fc_stats_snapshot_t snap;
    fc_stats_snapshot(stats, &snap);
    INTEG_ASSERT((int64_t)snap.bytes_sent == mx_gauge_get(g_sent),
                 "fc_stats bytes_sent matches gauge");
    INTEG_ASSERT(snap.stalls == (uint64_t)mx_gauge_get(g_stalls),
                 "fc_stats stalls matches gauge");

    /* ── teardown ── */
    fc_engine_destroy(engine);
    fc_stats_destroy(stats);
    mx_registry_destroy(reg);

    INTEG_PASS("flowctl↔metrics", "normal flow — sends wire to gauges correctly");
    return 0;
}

/* ─────────────────────────────────────────────────────────────────── *
 * Integration test 2: stall path — credit exhausted, gauge increments.
 * ─────────────────────────────────────────────────────────────────── */
static int test_stall_path(void)
{
    INTEG_SUITE("flowctl↔metrics: stall path");

    fc_params_t p;
    fc_params_init(&p, 1000, 300, 1000, 100);   /* small budget = 300 bytes */
    fc_engine_t *engine = fc_engine_create(&p);
    fc_stats_t  *stats  = fc_stats_create();

    mx_registry_t *reg  = mx_registry_create();
    mx_gauge_t *g_sent   = mx_registry_register(reg, "fc_bytes_sent");
    mx_gauge_t *g_stalls = mx_registry_register(reg, "fc_stalls");
    mx_gauge_t *g_dropped = mx_registry_register(reg, "fc_bytes_dropped");

    /* Consume the entire 300-byte budget */
    attempt_send(engine, stats, g_sent, g_stalls, 300);
    INTEG_ASSERT(fc_engine_credit(engine) == 0, "budget exhausted");

    /* Next send should stall — gauge must increment */
    int ok = attempt_send(engine, stats, g_sent, g_stalls, 100);
    INTEG_ASSERT(ok == 0, "send after exhaustion stalls");
    INTEG_ASSERT(mx_gauge_get(g_stalls) == 1, "stall gauge = 1 after first stall");

    /* Second stall */
    ok = attempt_send(engine, stats, g_sent, g_stalls, 100);
    INTEG_ASSERT(ok == 0, "second send stalls");
    INTEG_ASSERT(mx_gauge_get(g_stalls) == 2, "stall gauge = 2 after second stall");

    /* Record a drop separately (caller decided to discard, not retry) */
    fc_stats_record_drop(stats, 100);
    mx_gauge_add(g_dropped, 100);
    INTEG_ASSERT(mx_gauge_get(g_dropped) == 100, "dropped bytes gauge wired");

    fc_engine_destroy(engine);
    fc_stats_destroy(stats);
    mx_registry_destroy(reg);

    INTEG_PASS("flowctl↔metrics", "stall path — stall gauge increments on credit exhaustion");
    return 0;
}

/* ─────────────────────────────────────────────────────────────────── *
 * Integration test 3: snapshot reflects all accumulated gauge state.
 * ─────────────────────────────────────────────────────────────────── */
static int test_snapshot_reflects_gauge_state(void)
{
    INTEG_SUITE("flowctl↔metrics: snapshot");

    fc_params_t p;
    fc_params_init(&p, 2000, 500, 2000, 100);
    fc_engine_t *engine = fc_engine_create(&p);
    fc_stats_t  *stats  = fc_stats_create();

    mx_registry_t *reg = mx_registry_create();
    mx_gauge_t *g_sent     = mx_registry_register(reg, "fc_bytes_sent");
    mx_gauge_t *g_stalls   = mx_registry_register(reg, "fc_stalls");
    mx_gauge_t *g_replenish = mx_registry_register(reg, "fc_replenish_count");

    /* Drive some activity */
    attempt_send(engine, stats, g_sent, g_stalls, 200);
    attempt_send(engine, stats, g_sent, g_stalls, 200);
    do_replenish(engine, stats, g_replenish, 400);
    attempt_send(engine, stats, g_sent, g_stalls, 100);

    /* Take a snapshot */
    mx_snapshot_t snap;
    mx_snapshot_init(&snap);
    snap.gauge_count = mx_registry_snapshot_all(reg, snap.gauges, MX_MAX_GAUGES);
    INTEG_ASSERT(snap.gauge_count == 3, "snapshot captured 3 gauges");

    /* Find fc_bytes_sent in the snapshot */
    int found = 0;
    for (int i = 0; i < snap.gauge_count; i++) {
        if (strncmp(snap.gauges[i].name, "fc_bytes_sent",
                    MX_GAUGE_NAME_MAX) == 0) {
            INTEG_ASSERT(snap.gauges[i].value == 500,
                         "snapshot fc_bytes_sent = 500");
            found = 1;
        }
    }
    INTEG_ASSERT(found == 1, "fc_bytes_sent found in snapshot");

    fc_engine_destroy(engine);
    fc_stats_destroy(stats);
    mx_registry_destroy(reg);

    INTEG_PASS("flowctl↔metrics",
               "snapshot — registry snapshot reflects all gauge state");
    return 0;
}

int main(void)
{
    int failures = 0;

    failures += test_normal_flow();
    failures += test_stall_path();
    failures += test_snapshot_reflects_gauge_state();

    printf("\n");
    if (failures == 0)
        printf("ALL FLOWCTL↔METRICS INTEGRATION TESTS PASSED\n");
    else
        printf("%d FLOWCTL↔METRICS INTEGRATION TEST(S) FAILED\n", failures);

    return failures ? 1 : 0;
}
