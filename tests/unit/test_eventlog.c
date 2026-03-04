/*
 * test_eventlog.c — Unit tests for PHASE-58 Circular Event Log
 *
 * Tests event_entry (encode/decode/level-names), event_ring
 * (push/get/count/clear/wrap-around/find_level), and event_export
 * (JSON array format, plain-text format, buffer-too-small).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/eventlog/event_entry.h"
#include "../../src/eventlog/event_ring.h"
#include "../../src/eventlog/event_export.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

/* ── event_entry tests ───────────────────────────────────────────── */

static int test_entry_roundtrip(void) {
    printf("\n=== test_entry_roundtrip ===\n");

    event_entry_t orig = {
        .timestamp_us = 9876543210ULL,
        .level        = EVENT_LEVEL_WARN,
        .event_type   = 0xABCD,
    };
    strncpy(orig.msg, "packet loss detected", EVENT_MSG_MAX - 1);

    uint8_t buf[EVENT_ENTRY_HDR_SIZE + EVENT_MSG_MAX];
    int n = event_entry_encode(&orig, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "encode positive");

    event_entry_t dec;
    int rc = event_entry_decode(buf, (size_t)n, &dec);
    TEST_ASSERT(rc == 0, "decode ok");
    TEST_ASSERT(dec.timestamp_us == 9876543210ULL, "timestamp");
    TEST_ASSERT(dec.level == EVENT_LEVEL_WARN, "level");
    TEST_ASSERT(dec.event_type == 0xABCD, "event_type");
    TEST_ASSERT(strcmp(dec.msg, "packet loss detected") == 0, "msg");

    TEST_PASS("event_entry encode/decode round-trip");
    return 0;
}

static int test_entry_level_names(void) {
    printf("\n=== test_entry_level_names ===\n");

    TEST_ASSERT(strcmp(event_level_name(EVENT_LEVEL_DEBUG), "DEBUG") == 0, "DEBUG");
    TEST_ASSERT(strcmp(event_level_name(EVENT_LEVEL_INFO),  "INFO")  == 0, "INFO");
    TEST_ASSERT(strcmp(event_level_name(EVENT_LEVEL_WARN),  "WARN")  == 0, "WARN");
    TEST_ASSERT(strcmp(event_level_name(EVENT_LEVEL_ERROR), "ERROR") == 0, "ERROR");
    TEST_ASSERT(strcmp(event_level_name((event_level_t)99), "UNKNOWN") == 0, "unknown");

    TEST_PASS("event_entry level names");
    return 0;
}

/* ── event_ring tests ────────────────────────────────────────────── */

static event_entry_t make_entry(uint64_t ts, event_level_t lvl, const char *msg) {
    event_entry_t e; memset(&e, 0, sizeof(e));
    e.timestamp_us = ts;
    e.level        = lvl;
    strncpy(e.msg, msg, EVENT_MSG_MAX - 1);
    return e;
}

static int test_ring_push_get(void) {
    printf("\n=== test_ring_push_get ===\n");

    event_ring_t *r = event_ring_create();
    TEST_ASSERT(r != NULL, "ring created");
    TEST_ASSERT(event_ring_is_empty(r), "initially empty");

    event_entry_t e1 = make_entry(100, EVENT_LEVEL_INFO,  "first");
    event_entry_t e2 = make_entry(200, EVENT_LEVEL_WARN,  "second");
    event_entry_t e3 = make_entry(300, EVENT_LEVEL_ERROR, "third");

    event_ring_push(r, &e1);
    event_ring_push(r, &e2);
    event_ring_push(r, &e3);
    TEST_ASSERT(event_ring_count(r) == 3, "count 3");

    event_entry_t out;
    /* age 0 = newest */
    event_ring_get(r, 0, &out);
    TEST_ASSERT(out.timestamp_us == 300 && strcmp(out.msg, "third") == 0, "newest = third");
    event_ring_get(r, 2, &out);
    TEST_ASSERT(out.timestamp_us == 100 && strcmp(out.msg, "first") == 0, "oldest = first");

    event_ring_clear(r);
    TEST_ASSERT(event_ring_is_empty(r), "empty after clear");

    event_ring_destroy(r);
    TEST_PASS("event_ring push/get/clear");
    return 0;
}

static int test_ring_wraparound(void) {
    printf("\n=== test_ring_wraparound ===\n");

    event_ring_t *r = event_ring_create();

    /* Fill beyond capacity */
    for (int i = 0; i < EVENT_RING_CAPACITY + 5; i++) {
        char msg[16];
        snprintf(msg, sizeof(msg), "ev%d", i);
        event_entry_t e = make_entry((uint64_t)i, EVENT_LEVEL_DEBUG, msg);
        event_ring_push(r, &e);
    }

    TEST_ASSERT(event_ring_count(r) == EVENT_RING_CAPACITY, "count capped at CAPACITY");

    event_entry_t newest;
    event_ring_get(r, 0, &newest);
    TEST_ASSERT(newest.timestamp_us == (uint64_t)(EVENT_RING_CAPACITY + 4),
                "newest is the last pushed entry");

    event_ring_destroy(r);
    TEST_PASS("event_ring wrap-around");
    return 0;
}

static int test_ring_find_level(void) {
    printf("\n=== test_ring_find_level ===\n");

    event_ring_t *r = event_ring_create();

    event_ring_push(r, &(event_entry_t){ .level = EVENT_LEVEL_DEBUG });
    event_ring_push(r, &(event_entry_t){ .level = EVENT_LEVEL_INFO  });
    event_ring_push(r, &(event_entry_t){ .level = EVENT_LEVEL_WARN  });
    event_ring_push(r, &(event_entry_t){ .level = EVENT_LEVEL_ERROR });
    event_ring_push(r, &(event_entry_t){ .level = EVENT_LEVEL_WARN  });

    int ages[10];
    int n = event_ring_find_level(r, EVENT_LEVEL_WARN, ages, 10);
    TEST_ASSERT(n == 3, "3 entries >= WARN (1 ERROR + 2 WARN)");

    n = event_ring_find_level(r, EVENT_LEVEL_ERROR, ages, 10);
    TEST_ASSERT(n == 1, "1 entry >= ERROR");

    event_ring_destroy(r);
    TEST_PASS("event_ring find_level");
    return 0;
}

/* ── event_export tests ──────────────────────────────────────────── */

static int test_export_json(void) {
    printf("\n=== test_export_json ===\n");

    event_ring_t *r = event_ring_create();
    event_entry_t e1 = make_entry(1000, EVENT_LEVEL_INFO, "started");
    event_entry_t e2 = make_entry(2000, EVENT_LEVEL_WARN, "slow");
    event_ring_push(r, &e1);
    event_ring_push(r, &e2);

    char buf[2048];
    int n = event_export_json(r, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "export JSON positive");
    TEST_ASSERT(buf[0] == '[', "starts with [");
    TEST_ASSERT(buf[n-1] == ']', "ends with ]");
    TEST_ASSERT(strstr(buf, "\"level\":\"WARN\"") != NULL, "WARN in JSON");
    TEST_ASSERT(strstr(buf, "\"msg\":\"slow\"") != NULL, "msg in JSON");

    /* Empty ring → [] */
    event_ring_clear(r);
    n = event_export_json(r, buf, sizeof(buf));
    TEST_ASSERT(n == 2 && strcmp(buf, "[]") == 0, "empty ring → []");

    event_ring_destroy(r);
    TEST_PASS("event_export JSON");
    return 0;
}

static int test_export_text(void) {
    printf("\n=== test_export_text ===\n");

    event_ring_t *r = event_ring_create();
    event_entry_t e = make_entry(500, EVENT_LEVEL_ERROR, "oops");
    event_ring_push(r, &e);

    char buf[512];
    int n = event_export_text(r, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "export text positive");
    TEST_ASSERT(strstr(buf, "[ERROR]") != NULL, "[ERROR] prefix");
    TEST_ASSERT(strstr(buf, "oops") != NULL, "message in output");

    /* Buffer too small → -1 */
    n = event_export_text(r, buf, 5);
    TEST_ASSERT(n == -1, "too-small buffer → -1");

    event_ring_destroy(r);
    TEST_PASS("event_export text");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_entry_roundtrip();
    failures += test_entry_level_names();

    failures += test_ring_push_get();
    failures += test_ring_wraparound();
    failures += test_ring_find_level();

    failures += test_export_json();
    failures += test_export_text();

    printf("\n");
    if (failures == 0)
        printf("ALL EVENTLOG TESTS PASSED\n");
    else
        printf("%d EVENTLOG TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
