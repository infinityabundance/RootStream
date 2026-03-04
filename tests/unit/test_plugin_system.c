/*
 * test_plugin_system.c — Unit tests for PHASE-35 plugin system
 *
 * Tests the plugin_api, plugin_loader, and plugin_registry without
 * requiring actual .so files on disk.  We exercise the registry's
 * in-memory state management directly.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/plugin/plugin_api.h"
#include "../../src/plugin/plugin_loader.h"
#include "../../src/plugin/plugin_registry.h"

/* ── Test macros (same style as existing tests) ─────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── Stub host log ──────────────────────────────────────────────── */

static void stub_log(const char *plugin_name,
                     const char *level,
                     const char *msg) {
    (void)plugin_name;
    (void)level;
    (void)msg;
    /* Silent in tests */
}

static plugin_host_api_t make_host(void) {
    plugin_host_api_t h;
    memset(&h, 0, sizeof(h));
    h.api_version = PLUGIN_API_VERSION;
    h.log         = stub_log;
    return h;
}

/* ── Tests ──────────────────────────────────────────────────────── */

static int test_descriptor_constants(void) {
    printf("\n=== test_descriptor_constants ===\n");

    TEST_ASSERT(PLUGIN_API_MAGIC   == 0x52535054U, "magic value correct");
    TEST_ASSERT(PLUGIN_API_VERSION >= 1,           "api version >= 1");

    TEST_PASS("descriptor constants");
    return 0;
}

static int test_plugin_type_enum(void) {
    printf("\n=== test_plugin_type_enum ===\n");

    TEST_ASSERT(PLUGIN_TYPE_UNKNOWN   == 0, "UNKNOWN == 0");
    TEST_ASSERT(PLUGIN_TYPE_ENCODER   == 1, "ENCODER == 1");
    TEST_ASSERT(PLUGIN_TYPE_DECODER   == 2, "DECODER == 2");
    TEST_ASSERT(PLUGIN_TYPE_CAPTURE   == 3, "CAPTURE == 3");
    TEST_ASSERT(PLUGIN_TYPE_FILTER    == 4, "FILTER == 4");
    TEST_ASSERT(PLUGIN_TYPE_TRANSPORT == 5, "TRANSPORT == 5");
    TEST_ASSERT(PLUGIN_TYPE_UI        == 6, "UI == 6");

    TEST_PASS("plugin_type enum values");
    return 0;
}

static int test_registry_create_destroy(void) {
    printf("\n=== test_registry_create_destroy ===\n");

    plugin_host_api_t host = make_host();

    plugin_registry_t *reg = plugin_registry_create(&host);
    TEST_ASSERT(reg != NULL, "registry created");

    TEST_ASSERT(plugin_registry_count(reg) == 0, "initial count == 0");

    plugin_registry_destroy(reg);
    TEST_PASS("registry create/destroy");
    return 0;
}

static int test_registry_create_null(void) {
    printf("\n=== test_registry_create_null ===\n");

    plugin_registry_t *reg = plugin_registry_create(NULL);
    TEST_ASSERT(reg == NULL, "NULL host returns NULL registry");

    TEST_PASS("registry NULL host guard");
    return 0;
}

static int test_registry_load_nonexistent(void) {
    printf("\n=== test_registry_load_nonexistent ===\n");

    plugin_host_api_t host = make_host();
    plugin_registry_t *reg = plugin_registry_create(&host);
    TEST_ASSERT(reg != NULL, "registry created");

    int rc = plugin_registry_load(reg, "/nonexistent/path/fake_plugin.so");
    TEST_ASSERT(rc == -1, "loading nonexistent path returns -1");
    TEST_ASSERT(plugin_registry_count(reg) == 0, "count stays 0 on failure");

    plugin_registry_destroy(reg);
    TEST_PASS("registry load nonexistent path");
    return 0;
}

static int test_registry_find_empty(void) {
    printf("\n=== test_registry_find_empty ===\n");

    plugin_host_api_t host = make_host();
    plugin_registry_t *reg = plugin_registry_create(&host);
    TEST_ASSERT(reg != NULL, "registry created");

    plugin_handle_t *h;
    h = plugin_registry_find_by_name(reg, "anything");
    TEST_ASSERT(h == NULL, "find_by_name returns NULL on empty registry");

    h = plugin_registry_find_by_type(reg, PLUGIN_TYPE_ENCODER);
    TEST_ASSERT(h == NULL, "find_by_type returns NULL on empty registry");

    h = plugin_registry_get(reg, 0);
    TEST_ASSERT(h == NULL, "get(0) returns NULL on empty registry");

    plugin_registry_destroy(reg);
    TEST_PASS("registry find on empty registry");
    return 0;
}

static int test_registry_scan_empty_dir(void) {
    printf("\n=== test_registry_scan_empty_dir ===\n");

    plugin_host_api_t host = make_host();
    plugin_registry_t *reg = plugin_registry_create(&host);
    TEST_ASSERT(reg != NULL, "registry created");

    /* /tmp always exists but contains no rootstream plugins */
    int n = plugin_registry_scan_dir(reg, "/tmp");
    /* n >= 0; no plugins expected in /tmp */
    TEST_ASSERT(n >= 0, "scan_dir returns non-negative count");

    plugin_registry_destroy(reg);
    TEST_PASS("registry scan empty/benign dir");
    return 0;
}

static int test_registry_unload_missing(void) {
    printf("\n=== test_registry_unload_missing ===\n");

    plugin_host_api_t host = make_host();
    plugin_registry_t *reg = plugin_registry_create(&host);
    TEST_ASSERT(reg != NULL, "registry created");

    int rc = plugin_registry_unload(reg, "no_such_plugin");
    TEST_ASSERT(rc == -1, "unload returns -1 when plugin not found");

    plugin_registry_destroy(reg);
    TEST_PASS("registry unload missing plugin");
    return 0;
}

static int test_loader_null_args(void) {
    printf("\n=== test_loader_null_args ===\n");

    plugin_host_api_t host = make_host();

    TEST_ASSERT(plugin_loader_load(NULL, &host) == NULL,
                "load NULL path returns NULL");
    TEST_ASSERT(plugin_loader_load("/tmp/x.so", NULL) == NULL,
                "load NULL host returns NULL");

    /* Unload NULL is a no-op (must not crash) */
    plugin_loader_unload(NULL);

    TEST_ASSERT(plugin_loader_get_descriptor(NULL) == NULL,
                "get_descriptor NULL returns NULL");
    TEST_ASSERT(plugin_loader_get_path(NULL) == NULL,
                "get_path NULL returns NULL");

    TEST_PASS("loader NULL argument guards");
    return 0;
}

static int test_host_api_version(void) {
    printf("\n=== test_host_api_version ===\n");

    plugin_host_api_t host = make_host();
    TEST_ASSERT(host.api_version == PLUGIN_API_VERSION,
                "host api_version set correctly");

    TEST_PASS("host API version");
    return 0;
}

/* ── main ───────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_descriptor_constants();
    failures += test_plugin_type_enum();
    failures += test_registry_create_destroy();
    failures += test_registry_create_null();
    failures += test_registry_load_nonexistent();
    failures += test_registry_find_empty();
    failures += test_registry_scan_empty_dir();
    failures += test_registry_unload_missing();
    failures += test_loader_null_args();
    failures += test_host_api_version();

    printf("\n");
    if (failures == 0) {
        printf("ALL PLUGIN SYSTEM TESTS PASSED\n");
    } else {
        printf("%d PLUGIN SYSTEM TEST(S) FAILED\n", failures);
    }
    return failures ? 1 : 0;
}
