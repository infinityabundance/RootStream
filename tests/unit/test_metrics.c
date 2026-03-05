/*
 * test_metrics.c — Unit tests for PHASE-80 Metrics Exporter
 *
 * Tests mx_gauge (init/set/add/get/reset), mx_registry
 * (register/lookup/duplicate/count/cap/snapshot_all), and
 * mx_snapshot (init/dump).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../../src/metrics/mx_gauge.h"
#include "../../src/metrics/mx_registry.h"
#include "../../src/metrics/mx_snapshot.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── mx_gauge ────────────────────────────────────────────────────── */

static int test_gauge(void) {
    printf("\n=== test_gauge ===\n");

    mx_gauge_t g;
    TEST_ASSERT(mx_gauge_init(&g, "fps") == 0, "init ok");
    TEST_ASSERT(strcmp(g.name, "fps") == 0, "name");
    TEST_ASSERT(g.value == 0, "initial value = 0");

    mx_gauge_set(&g, 60);
    TEST_ASSERT(mx_gauge_get(&g) == 60, "set → 60");

    mx_gauge_add(&g, -5);
    TEST_ASSERT(mx_gauge_get(&g) == 55, "add -5 → 55");

    mx_gauge_reset(&g);
    TEST_ASSERT(mx_gauge_get(&g) == 0, "reset → 0");

    TEST_ASSERT(mx_gauge_init(NULL, "x") == -1, "NULL → -1");
    TEST_ASSERT(mx_gauge_init(&g, "") == -1, "empty name → -1");
    TEST_ASSERT(mx_gauge_get(NULL) == 0, "get(NULL) = 0");

    TEST_PASS("mx_gauge init/set/add/get/reset/null-guards");
    return 0;
}

/* ── mx_registry ─────────────────────────────────────────────────── */

static int test_registry(void) {
    printf("\n=== test_registry ===\n");

    mx_registry_t *r = mx_registry_create();
    TEST_ASSERT(r != NULL, "created");
    TEST_ASSERT(mx_registry_count(r) == 0, "initially 0");

    mx_gauge_t *g1 = mx_registry_register(r, "latency_us");
    mx_gauge_t *g2 = mx_registry_register(r, "bitrate_kbps");
    TEST_ASSERT(g1 != NULL, "register g1");
    TEST_ASSERT(g2 != NULL, "register g2");
    TEST_ASSERT(mx_registry_count(r) == 2, "count = 2");

    /* Duplicate name */
    TEST_ASSERT(mx_registry_register(r, "latency_us") == NULL, "duplicate → NULL");

    /* lookup */
    TEST_ASSERT(mx_registry_lookup(r, "latency_us")   == g1, "lookup g1");
    TEST_ASSERT(mx_registry_lookup(r, "bitrate_kbps") == g2, "lookup g2");
    TEST_ASSERT(mx_registry_lookup(r, "unknown")      == NULL, "unknown → NULL");

    /* Mutate via pointer and check */
    mx_gauge_set(g1, 5000);
    TEST_ASSERT(mx_gauge_get(mx_registry_lookup(r, "latency_us")) == 5000,
                "mutation visible via lookup");

    /* snapshot_all */
    mx_gauge_t snap[MX_MAX_GAUGES];
    int n = mx_registry_snapshot_all(r, snap, MX_MAX_GAUGES);
    TEST_ASSERT(n == 2, "snapshot_all returns 2");
    /* Both gauge names present */
    int found_lat = 0, found_bit = 0;
    for (int i = 0; i < n; i++) {
        if (strcmp(snap[i].name, "latency_us")   == 0) found_lat = 1;
        if (strcmp(snap[i].name, "bitrate_kbps") == 0) found_bit = 1;
    }
    TEST_ASSERT(found_lat && found_bit, "both names in snapshot");

    mx_registry_destroy(r);
    TEST_PASS("mx_registry register/lookup/duplicate/snapshot_all");
    return 0;
}

/* ── mx_snapshot ─────────────────────────────────────────────────── */

static int test_snapshot(void) {
    printf("\n=== test_snapshot ===\n");

    mx_snapshot_t s;
    TEST_ASSERT(mx_snapshot_init(&s) == 0, "init ok");
    TEST_ASSERT(s.gauge_count == 0, "initially 0 gauges");

    /* Populate manually */
    mx_gauge_init(&s.gauges[0], "a");
    mx_gauge_set(&s.gauges[0], 100);
    mx_gauge_init(&s.gauges[1], "b");
    mx_gauge_set(&s.gauges[1], 200);
    s.gauge_count = 2;
    s.timestamp_us = 9999;

    mx_gauge_t out[4];
    int n = mx_snapshot_dump(&s, out, 4);
    TEST_ASSERT(n == 2, "dump 2 gauges");
    TEST_ASSERT(mx_gauge_get(&out[0]) == 100, "out[0] = 100");
    TEST_ASSERT(mx_gauge_get(&out[1]) == 200, "out[1] = 200");

    /* Truncation */
    n = mx_snapshot_dump(&s, out, 1);
    TEST_ASSERT(n == 1, "truncation at max_out=1");

    TEST_ASSERT(mx_snapshot_init(NULL) == -1, "NULL → -1");
    TEST_ASSERT(mx_snapshot_dump(NULL, out, 4) == -1, "dump NULL s → -1");

    TEST_PASS("mx_snapshot init/dump/truncation/null-guards");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_gauge();
    failures += test_registry();
    failures += test_snapshot();

    printf("\n");
    if (failures == 0) printf("ALL METRICS TESTS PASSED\n");
    else               printf("%d METRICS TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
