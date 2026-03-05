/*
 * test_health.c — Unit tests for PHASE-63 Stream Health Monitor
 *
 * Tests health_metric (init/set_fval/set_uval/set_bval/evaluate/names),
 * health_monitor (register/get/dup-guard/full-guard/evaluate/summary),
 * and health_report (JSON serialiser output contains expected keys).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "../../src/health/health_metric.h"
#include "../../src/health/health_monitor.h"
#include "../../src/health/health_report.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

/* ── health_metric ───────────────────────────────────────────────── */

static int test_metric_init(void) {
    printf("\n=== test_metric_init ===\n");

    health_metric_t m;
    TEST_ASSERT(hm_init(&m, "cpu", HM_GAUGE) == 0, "init ok");
    TEST_ASSERT(strcmp(m.name, "cpu") == 0, "name");
    TEST_ASSERT(m.kind == HM_GAUGE, "kind GAUGE");
    TEST_ASSERT(!m.has_threshold, "no threshold");

    TEST_ASSERT(hm_init(NULL, "x", HM_GAUGE) == -1, "NULL → -1");
    TEST_PASS("health_metric init");
    return 0;
}

static int test_metric_evaluate(void) {
    printf("\n=== test_metric_evaluate ===\n");

    health_metric_t m;
    hm_init(&m, "fps", HM_GAUGE);

    /* No threshold → always OK */
    hm_set_fval(&m, -999.0);
    TEST_ASSERT(hm_evaluate(&m) == HM_OK, "no threshold → OK");

    /* Add thresholds: warn if < 15 or > 90; crit if < 5 or > 100 */
    hm_threshold_t t = { .warn_lo=15, .warn_hi=90, .crit_lo=5, .crit_hi=100 };
    hm_set_threshold(&m, &t);

    hm_set_fval(&m, 30.0);  TEST_ASSERT(hm_evaluate(&m) == HM_OK,   "30 → OK");
    hm_set_fval(&m, 10.0);  TEST_ASSERT(hm_evaluate(&m) == HM_WARN, "10 → WARN");
    hm_set_fval(&m, 3.0);   TEST_ASSERT(hm_evaluate(&m) == HM_CRIT, "3 → CRIT");
    hm_set_fval(&m, 99.0);  TEST_ASSERT(hm_evaluate(&m) == HM_WARN, "99 → WARN");
    hm_set_fval(&m, 105.0); TEST_ASSERT(hm_evaluate(&m) == HM_CRIT, "105 → CRIT");

    /* BOOLEAN: true → OK, false → CRIT */
    health_metric_t bm;
    hm_init(&bm, "enc_ok", HM_BOOLEAN);
    hm_threshold_t bt = {0,0,0,0};
    hm_set_threshold(&bm, &bt);
    hm_set_bval(&bm, true);  TEST_ASSERT(hm_evaluate(&bm) == HM_OK,   "true → OK");
    hm_set_bval(&bm, false); TEST_ASSERT(hm_evaluate(&bm) == HM_CRIT, "false → CRIT");

    TEST_ASSERT(strcmp(hm_level_name(HM_OK),   "OK")   == 0, "level OK");
    TEST_ASSERT(strcmp(hm_level_name(HM_WARN),  "WARN") == 0, "level WARN");
    TEST_ASSERT(strcmp(hm_level_name(HM_CRIT),  "CRIT") == 0, "level CRIT");
    TEST_ASSERT(strcmp(hm_kind_name(HM_RATE), "RATE")   == 0, "kind RATE");

    TEST_PASS("health_metric evaluate + names");
    return 0;
}

/* ── health_monitor ──────────────────────────────────────────────── */

static int test_monitor_register(void) {
    printf("\n=== test_monitor_register ===\n");

    health_monitor_t *hm = health_monitor_create();
    TEST_ASSERT(hm != NULL, "created");
    TEST_ASSERT(health_monitor_metric_count(hm) == 0, "initially 0");

    health_metric_t *m = health_monitor_register(hm, "cpu", HM_GAUGE);
    TEST_ASSERT(m != NULL, "registered cpu");
    TEST_ASSERT(health_monitor_metric_count(hm) == 1, "1 metric");

    /* Duplicate name */
    TEST_ASSERT(health_monitor_register(hm, "cpu", HM_GAUGE) == NULL, "dup → NULL");

    /* Get by name */
    health_metric_t *got = health_monitor_get(hm, "cpu");
    TEST_ASSERT(got == m, "get returns same pointer");
    TEST_ASSERT(health_monitor_get(hm, "nonexistent") == NULL, "unknown → NULL");

    health_monitor_destroy(hm);
    TEST_PASS("health_monitor register/get/dup-guard");
    return 0;
}

static int test_monitor_evaluate(void) {
    printf("\n=== test_monitor_evaluate ===\n");

    health_monitor_t *hm = health_monitor_create();

    /* Register 3 metrics */
    health_metric_t *cpu = health_monitor_register(hm, "cpu", HM_GAUGE);
    health_metric_t *mem = health_monitor_register(hm, "mem", HM_GAUGE);
    health_metric_t *enc = health_monitor_register(hm, "enc", HM_BOOLEAN);

    /* Set thresholds */
    hm_threshold_t tc = { .warn_lo=0, .warn_hi=80, .crit_lo=0, .crit_hi=95 };
    hm_set_threshold(cpu, &tc);
    hm_set_threshold(mem, &tc);
    hm_threshold_t tb = {0,0,0,0};
    hm_set_threshold(enc, &tb);

    /* All OK */
    hm_set_fval(cpu, 50.0); hm_set_fval(mem, 60.0); hm_set_bval(enc, true);
    health_summary_t s;
    health_monitor_evaluate(hm, &s);
    TEST_ASSERT(s.overall == HM_OK, "all OK → overall OK");
    TEST_ASSERT(s.n_ok == 3, "3 OK");

    /* cpu in WARN range */
    hm_set_fval(cpu, 85.0);
    health_monitor_evaluate(hm, &s);
    TEST_ASSERT(s.overall == HM_WARN, "1 WARN → overall WARN");
    TEST_ASSERT(s.n_warn == 1 && s.n_ok == 2, "1 WARN, 2 OK");

    /* enc CRIT */
    hm_set_bval(enc, false);
    health_monitor_evaluate(hm, &s);
    TEST_ASSERT(s.overall == HM_CRIT, "1 CRIT → overall CRIT");
    TEST_ASSERT(s.n_crit == 1, "1 CRIT");

    health_monitor_destroy(hm);
    TEST_PASS("health_monitor evaluate / summary");
    return 0;
}

/* ── health_report ───────────────────────────────────────────────── */

static int test_report_json(void) {
    printf("\n=== test_report_json ===\n");

    health_monitor_t *hm = health_monitor_create();
    health_metric_t *fps = health_monitor_register(hm, "fps", HM_RATE);
    hm_set_fval(fps, 30.0);

    char buf[4096];
    int n = health_report_json(hm, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "report returned bytes");
    TEST_ASSERT(strstr(buf, "\"overall\"") != NULL, "has 'overall'");
    TEST_ASSERT(strstr(buf, "\"metrics\"") != NULL, "has 'metrics'");
    TEST_ASSERT(strstr(buf, "fps")         != NULL, "has metric name");

    health_monitor_destroy(hm);
    TEST_PASS("health_report_json basic output");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_metric_init();
    failures += test_metric_evaluate();
    failures += test_monitor_register();
    failures += test_monitor_evaluate();
    failures += test_report_json();

    printf("\n");
    if (failures == 0) printf("ALL HEALTH TESTS PASSED\n");
    else               printf("%d HEALTH TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
