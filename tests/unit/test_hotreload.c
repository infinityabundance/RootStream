/*
 * test_hotreload.c — Unit tests for PHASE-66 Plugin Hot-Reload Manager
 *
 * Tests hr_entry (init/clear/state names), hr_manager (register/dup-guard/
 * full-guard/load/reload/unload/get/version-bump), and hr_stats
 * (reload/fail/loaded/snapshot/reset).
 *
 * All tests use stub dlopen/dlclose — no real shared libraries required.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../../src/hotreload/hr_entry.h"
#include "../../src/hotreload/hr_manager.h"
#include "../../src/hotreload/hr_stats.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

/* ── stub dl ─────────────────────────────────────────────────────── */

static int stub_close_count = 0;
static void *test_dlopen(const char *p, int f) { (void)f; return p ? (void*)0xBEEF : NULL; }
static int   test_dlclose(void *h)  { (void)h; stub_close_count++; return 0; }
static void *fail_dlopen(const char *p, int f) { (void)p; (void)f; return NULL; }

/* ── hr_entry ────────────────────────────────────────────────────── */

static int test_entry_init(void) {
    printf("\n=== test_entry_init ===\n");

    hr_entry_t e;
    TEST_ASSERT(hr_entry_init(&e, "/lib/plugin.so") == 0, "init ok");
    TEST_ASSERT(strcmp(e.path, "/lib/plugin.so") == 0, "path");
    TEST_ASSERT(e.state == HR_STATE_UNLOADED, "initially UNLOADED");
    TEST_ASSERT(e.version == 0, "version 0");
    TEST_ASSERT(e.handle == NULL, "handle NULL");

    hr_entry_clear(&e);
    TEST_ASSERT(e.state == HR_STATE_UNLOADED, "clear → UNLOADED");
    TEST_ASSERT(strcmp(e.path, "/lib/plugin.so") == 0, "path preserved after clear");

    TEST_ASSERT(strcmp(hr_state_name(HR_STATE_LOADED),   "LOADED")   == 0, "LOADED");
    TEST_ASSERT(strcmp(hr_state_name(HR_STATE_FAILED),   "FAILED")   == 0, "FAILED");
    TEST_ASSERT(strcmp(hr_state_name(HR_STATE_UNLOADED), "UNLOADED") == 0, "UNLOADED");

    TEST_PASS("hr_entry init/clear/state names");
    return 0;
}

/* ── hr_manager ──────────────────────────────────────────────────── */

static int test_manager_register(void) {
    printf("\n=== test_manager_register ===\n");

    hr_manager_t *mgr = hr_manager_create(test_dlopen, test_dlclose);
    TEST_ASSERT(mgr != NULL, "created");
    TEST_ASSERT(hr_manager_plugin_count(mgr) == 0, "initially 0");

    TEST_ASSERT(hr_manager_register(mgr, "/lib/a.so") == 0, "register a");
    TEST_ASSERT(hr_manager_register(mgr, "/lib/b.so") == 0, "register b");
    TEST_ASSERT(hr_manager_plugin_count(mgr) == 2, "2 plugins");

    /* Duplicate path */
    TEST_ASSERT(hr_manager_register(mgr, "/lib/a.so") == -1, "dup → -1");

    hr_manager_destroy(mgr);
    TEST_PASS("hr_manager register / dup-guard");
    return 0;
}

static int test_manager_load_reload(void) {
    printf("\n=== test_manager_load_reload ===\n");

    hr_manager_t *mgr = hr_manager_create(test_dlopen, test_dlclose);
    hr_manager_register(mgr, "/lib/plug.so");

    /* Load */
    int rc = hr_manager_load(mgr, "/lib/plug.so", 1000);
    TEST_ASSERT(rc == 0, "load ok");
    const hr_entry_t *e = hr_manager_get(mgr, "/lib/plug.so");
    TEST_ASSERT(e != NULL, "get returns entry");
    TEST_ASSERT(e->state == HR_STATE_LOADED, "state LOADED");
    TEST_ASSERT(e->version == 1, "version = 1");
    TEST_ASSERT(e->last_load_us == 1000, "last_load_us");

    /* Reload */
    stub_close_count = 0;
    rc = hr_manager_reload(mgr, "/lib/plug.so", 2000);
    TEST_ASSERT(rc == 0, "reload ok");
    TEST_ASSERT(stub_close_count == 1, "dlclose called once");
    TEST_ASSERT(e->version == 2, "version bumped to 2");
    TEST_ASSERT(e->last_load_us == 2000, "timestamp updated");

    /* Unload */
    rc = hr_manager_unload(mgr, "/lib/plug.so");
    TEST_ASSERT(rc == 0, "unload ok");
    TEST_ASSERT(e->state == HR_STATE_UNLOADED, "state UNLOADED after unload");

    hr_manager_destroy(mgr);
    TEST_PASS("hr_manager load/reload/unload/version");
    return 0;
}

static int test_manager_load_fail(void) {
    printf("\n=== test_manager_load_fail ===\n");

    hr_manager_t *mgr = hr_manager_create(fail_dlopen, test_dlclose);
    hr_manager_register(mgr, "/lib/bad.so");

    int rc = hr_manager_load(mgr, "/lib/bad.so", 0);
    TEST_ASSERT(rc == -1, "failed load → -1");
    const hr_entry_t *e = hr_manager_get(mgr, "/lib/bad.so");
    TEST_ASSERT(e->state == HR_STATE_FAILED, "state FAILED");

    hr_manager_destroy(mgr);
    TEST_PASS("hr_manager failed load sets FAILED state");
    return 0;
}

/* ── hr_stats ────────────────────────────────────────────────────── */

static int test_hr_stats(void) {
    printf("\n=== test_hr_stats ===\n");

    hr_stats_t *st = hr_stats_create();
    TEST_ASSERT(st != NULL, "created");

    hr_stats_record_reload(st, 1, 1000);
    hr_stats_record_reload(st, 1, 2000);
    hr_stats_record_reload(st, 0, 3000); /* failure */
    hr_stats_set_loaded(st, 2);

    hr_stats_snapshot_t snap;
    int rc = hr_stats_snapshot(st, &snap);
    TEST_ASSERT(rc == 0, "snapshot ok");
    TEST_ASSERT(snap.reload_count == 2, "2 successes");
    TEST_ASSERT(snap.fail_count   == 1, "1 failure");
    TEST_ASSERT(snap.last_reload_us == 2000, "last reload ts = 2000");
    TEST_ASSERT(snap.loaded_plugins == 2, "2 loaded plugins");

    hr_stats_reset(st);
    hr_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.reload_count == 0, "reset ok");

    hr_stats_destroy(st);
    TEST_PASS("hr_stats reload/fail/loaded/snapshot/reset");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_entry_init();
    failures += test_manager_register();
    failures += test_manager_load_reload();
    failures += test_manager_load_fail();
    failures += test_hr_stats();

    printf("\n");
    if (failures == 0) printf("ALL HOTRELOAD TESTS PASSED\n");
    else               printf("%d HOTRELOAD TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
