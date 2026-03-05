/*
 * test_output.c — Unit tests for PHASE-68 Output Target Registry
 *
 * Tests output_target (init/state names), output_registry
 * (add/remove/get/dup-guard/full-guard/enable/disable/set_state/
 * active_count/foreach), and output_stats
 * (bytes/connect/error/active/snapshot/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../../src/output/output_target.h"
#include "../../src/output/output_registry.h"
#include "../../src/output/output_stats.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

/* ── output_target ───────────────────────────────────────────────── */

static int test_target_init(void) {
    printf("\n=== test_target_init ===\n");

    output_target_t t;
    TEST_ASSERT(ot_init(&t, "main", "rtmp://live/stream", "rtmp") == 0, "init ok");
    TEST_ASSERT(strcmp(t.name, "main") == 0, "name");
    TEST_ASSERT(strcmp(t.protocol, "rtmp") == 0, "protocol");
    TEST_ASSERT(t.state == OT_IDLE, "initially IDLE");
    TEST_ASSERT(t.enabled, "initially enabled");

    TEST_ASSERT(ot_init(NULL, "x", "u", "p") == -1, "NULL → -1");

    TEST_ASSERT(strcmp(ot_state_name(OT_IDLE),     "IDLE")     == 0, "IDLE");
    TEST_ASSERT(strcmp(ot_state_name(OT_ACTIVE),   "ACTIVE")   == 0, "ACTIVE");
    TEST_ASSERT(strcmp(ot_state_name(OT_ERROR),    "ERROR")    == 0, "ERROR");
    TEST_ASSERT(strcmp(ot_state_name(OT_DISABLED), "DISABLED") == 0, "DISABLED");

    TEST_PASS("output_target init / state names");
    return 0;
}

/* ── output_registry ─────────────────────────────────────────────── */

static int test_registry_add_remove(void) {
    printf("\n=== test_registry_add_remove ===\n");

    output_registry_t *r = output_registry_create();
    TEST_ASSERT(r != NULL, "created");
    TEST_ASSERT(output_registry_count(r) == 0, "initially 0");

    output_target_t *t = output_registry_add(r, "rtmp1", "rtmp://a/b", "rtmp");
    TEST_ASSERT(t != NULL, "added rtmp1");
    TEST_ASSERT(output_registry_count(r) == 1, "count = 1");

    /* Duplicate name */
    TEST_ASSERT(output_registry_add(r, "rtmp1", "u", "p") == NULL, "dup → NULL");

    /* Get */
    output_target_t *got = output_registry_get(r, "rtmp1");
    TEST_ASSERT(got == t, "get returns same ptr");
    TEST_ASSERT(output_registry_get(r, "unknown") == NULL, "unknown → NULL");

    /* Remove */
    TEST_ASSERT(output_registry_remove(r, "rtmp1") == 0, "remove ok");
    TEST_ASSERT(output_registry_count(r) == 0, "count back to 0");
    TEST_ASSERT(output_registry_remove(r, "rtmp1") == -1, "remove missing → -1");

    output_registry_destroy(r);
    TEST_PASS("output_registry add/remove/get/dup-guard");
    return 0;
}

static int test_registry_enable_disable(void) {
    printf("\n=== test_registry_enable_disable ===\n");

    output_registry_t *r = output_registry_create();
    output_registry_add(r, "a", "rtmp://a", "rtmp");
    output_registry_add(r, "b", "srt://b",  "srt");

    /* Set state to ACTIVE */
    output_registry_set_state(r, "a", OT_ACTIVE);
    output_registry_set_state(r, "b", OT_ACTIVE);
    TEST_ASSERT(output_registry_active_count(r) == 2, "2 active");

    /* Disable one */
    output_registry_disable(r, "a");
    output_target_t *ta = output_registry_get(r, "a");
    TEST_ASSERT(ta->state == OT_DISABLED, "a → DISABLED");
    TEST_ASSERT(!ta->enabled, "a disabled");
    TEST_ASSERT(output_registry_active_count(r) == 1, "1 active after disable");

    /* Re-enable */
    output_registry_enable(r, "a");
    TEST_ASSERT(ta->enabled, "a re-enabled");
    TEST_ASSERT(ta->state == OT_IDLE, "a state → IDLE after enable");

    output_registry_destroy(r);
    TEST_PASS("output_registry enable/disable/active_count");
    return 0;
}

/* Count callback helper */
static void count_cb(output_target_t *t, void *user) {
    (void)t;
    (*(int *)user)++;
}

static int test_registry_foreach(void) {
    printf("\n=== test_registry_foreach ===\n");

    output_registry_t *r = output_registry_create();
    output_registry_add(r, "x", "u1", "rtmp");
    output_registry_add(r, "y", "u2", "srt");
    output_registry_add(r, "z", "u3", "hls");

    int count = 0;
    output_registry_foreach(r, count_cb, &count);
    TEST_ASSERT(count == 3, "foreach visits 3 targets");

    output_registry_destroy(r);
    TEST_PASS("output_registry foreach");
    return 0;
}

/* ── output_stats ────────────────────────────────────────────────── */

static int test_output_stats(void) {
    printf("\n=== test_output_stats ===\n");

    output_stats_t *st = output_stats_create();
    TEST_ASSERT(st != NULL, "created");

    output_stats_record_bytes(st, 1000);
    output_stats_record_bytes(st, 2000);
    output_stats_record_connect(st);
    output_stats_record_connect(st);
    output_stats_record_error(st);
    output_stats_set_active(st, 3);

    output_stats_snapshot_t snap;
    TEST_ASSERT(output_stats_snapshot(st, &snap) == 0, "snapshot ok");
    TEST_ASSERT(snap.bytes_sent    == 3000, "bytes 3000");
    TEST_ASSERT(snap.connect_count == 2,    "2 connects");
    TEST_ASSERT(snap.error_count   == 1,    "1 error");
    TEST_ASSERT(snap.active_count  == 3,    "3 active");

    output_stats_reset(st);
    output_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.bytes_sent == 0, "reset ok");

    output_stats_destroy(st);
    TEST_PASS("output_stats bytes/connect/error/active/snapshot/reset");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_target_init();
    failures += test_registry_add_remove();
    failures += test_registry_enable_disable();
    failures += test_registry_foreach();
    failures += test_output_stats();

    printf("\n");
    if (failures == 0) printf("ALL OUTPUT TESTS PASSED\n");
    else               printf("%d OUTPUT TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
