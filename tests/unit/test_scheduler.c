/*
 * test_scheduler.c — Unit tests for PHASE-43 Stream Scheduler
 *
 * Tests schedule_entry (encode/decode/is_enabled), scheduler engine
 * (add/remove/tick/repeat), schedule_store (save/load), and
 * schedule_clock (now/format). No network or stream hardware required.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/scheduler/schedule_entry.h"
#include "../../src/scheduler/scheduler.h"
#include "../../src/scheduler/schedule_store.h"
#include "../../src/scheduler/schedule_clock.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── schedule_entry tests ────────────────────────────────────────── */

static int test_entry_roundtrip(void) {
    printf("\n=== test_entry_roundtrip ===\n");

    schedule_entry_t orig;
    memset(&orig, 0, sizeof(orig));
    orig.start_us    = 1700000000000000ULL;
    orig.duration_us = 3600000000U; /* 1 hour */
    orig.source_type = SCHED_SOURCE_CAPTURE;
    orig.flags       = SCHED_FLAG_ENABLED;
    orig.title_len   = (uint16_t)strlen("Morning Stream");
    snprintf(orig.title, sizeof(orig.title), "Morning Stream");

    uint8_t buf[SCHEDULE_ENTRY_MAX_SZ];
    int n = schedule_entry_encode(&orig, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "encode returns positive size");
    TEST_ASSERT((size_t)n == schedule_entry_encoded_size(&orig),
                "size matches predicted");

    schedule_entry_t decoded;
    int rc = schedule_entry_decode(buf, (size_t)n, &decoded);
    TEST_ASSERT(rc == 0, "decode succeeds");
    TEST_ASSERT(decoded.start_us    == orig.start_us,    "start_us preserved");
    TEST_ASSERT(decoded.duration_us == orig.duration_us, "duration preserved");
    TEST_ASSERT(decoded.source_type == SCHED_SOURCE_CAPTURE, "source_type preserved");
    TEST_ASSERT(decoded.flags == SCHED_FLAG_ENABLED, "flags preserved");
    TEST_ASSERT(strcmp(decoded.title, "Morning Stream") == 0, "title preserved");

    TEST_PASS("schedule entry encode/decode round-trip");
    return 0;
}

static int test_entry_bad_magic(void) {
    printf("\n=== test_entry_bad_magic ===\n");

    uint8_t buf[32] = {0xFF, 0xFF, 0xFF, 0xFF};
    schedule_entry_t e;
    int rc = schedule_entry_decode(buf, sizeof(buf), &e);
    TEST_ASSERT(rc == -1, "bad magic returns -1");

    TEST_PASS("schedule entry rejects bad magic");
    return 0;
}

static int test_entry_is_enabled(void) {
    printf("\n=== test_entry_is_enabled ===\n");

    schedule_entry_t e;
    memset(&e, 0, sizeof(e));
    e.flags = SCHED_FLAG_ENABLED;
    TEST_ASSERT(schedule_entry_is_enabled(&e), "enabled flag detected");

    e.flags = 0;
    TEST_ASSERT(!schedule_entry_is_enabled(&e), "no flag: not enabled");
    TEST_ASSERT(!schedule_entry_is_enabled(NULL), "NULL: not enabled");

    TEST_PASS("schedule_entry_is_enabled");
    return 0;
}

static int test_entry_null_guards(void) {
    printf("\n=== test_entry_null_guards ===\n");

    uint8_t buf[64];
    TEST_ASSERT(schedule_entry_encode(NULL, buf, sizeof(buf)) == -1,
                "encode NULL entry returns -1");
    schedule_entry_t e; memset(&e, 0, sizeof(e));
    TEST_ASSERT(schedule_entry_encode(&e, NULL, 0) == -1,
                "encode NULL buf returns -1");
    TEST_ASSERT(schedule_entry_decode(NULL, 0, &e) == -1,
                "decode NULL buf returns -1");

    TEST_PASS("schedule entry NULL guards");
    return 0;
}

/* ── scheduler tests ─────────────────────────────────────────────── */

static int fire_count_g = 0;
static uint64_t last_fired_id_g = 0;

static void on_fire(const schedule_entry_t *e, void *ud) {
    (void)ud;
    fire_count_g++;
    last_fired_id_g = e->id;
}

static schedule_entry_t make_sched_entry(uint64_t start_us,
                                          const char *title) {
    schedule_entry_t e;
    memset(&e, 0, sizeof(e));
    e.start_us    = start_us;
    e.duration_us = 1800000000U;
    e.source_type = SCHED_SOURCE_CAPTURE;
    e.flags       = SCHED_FLAG_ENABLED;
    e.title_len   = (uint16_t)strlen(title);
    snprintf(e.title, sizeof(e.title), "%s", title);
    return e;
}

static int test_scheduler_create(void) {
    printf("\n=== test_scheduler_create ===\n");

    scheduler_t *s = scheduler_create(NULL, NULL);
    TEST_ASSERT(s != NULL, "scheduler created");
    TEST_ASSERT(scheduler_count(s) == 0, "initial count 0");
    scheduler_destroy(s);
    scheduler_destroy(NULL); /* must not crash */

    TEST_PASS("scheduler create/destroy");
    return 0;
}

static int test_scheduler_add_remove(void) {
    printf("\n=== test_scheduler_add_remove ===\n");

    scheduler_t *s = scheduler_create(NULL, NULL);
    schedule_entry_t e = make_sched_entry(1000000000ULL, "Test");
    uint64_t id = scheduler_add(s, &e);
    TEST_ASSERT(id >= 1, "add returns valid id");
    TEST_ASSERT(scheduler_count(s) == 1, "count 1");

    schedule_entry_t got;
    int rc = scheduler_get(s, id, &got);
    TEST_ASSERT(rc == 0, "get succeeds");
    TEST_ASSERT(got.id == id, "id matches");
    TEST_ASSERT(strcmp(got.title, "Test") == 0, "title preserved");

    rc = scheduler_remove(s, id);
    TEST_ASSERT(rc == 0, "remove returns 0");
    TEST_ASSERT(scheduler_count(s) == 0, "count 0 after remove");

    rc = scheduler_remove(s, 9999);
    TEST_ASSERT(rc == -1, "remove nonexistent returns -1");

    scheduler_destroy(s);
    TEST_PASS("scheduler add/remove");
    return 0;
}

static int test_scheduler_tick_fires(void) {
    printf("\n=== test_scheduler_tick_fires ===\n");

    fire_count_g   = 0;
    last_fired_id_g = 0;

    scheduler_t *s = scheduler_create(on_fire, NULL);
    schedule_entry_t e = make_sched_entry(1000ULL, "Now");
    uint64_t id = scheduler_add(s, &e);
    TEST_ASSERT(id >= 1, "entry added");

    /* Tick at t=500 — should NOT fire (start=1000) */
    int fired = scheduler_tick(s, 500ULL);
    TEST_ASSERT(fired == 0, "no fire before start time");
    TEST_ASSERT(fire_count_g == 0, "callback not called");

    /* Tick at t=1000 — should fire */
    fired = scheduler_tick(s, 1000ULL);
    TEST_ASSERT(fired == 1, "fires at start time");
    TEST_ASSERT(fire_count_g == 1, "callback called once");
    TEST_ASSERT(last_fired_id_g == id, "correct entry fired");

    /* Entry is one-shot; should be removed from active list */
    TEST_ASSERT(scheduler_count(s) == 0, "one-shot entry removed after fire");

    scheduler_destroy(s);
    TEST_PASS("scheduler tick fires entry");
    return 0;
}

static int test_scheduler_tick_disabled(void) {
    printf("\n=== test_scheduler_tick_disabled ===\n");

    fire_count_g = 0;
    scheduler_t *s = scheduler_create(on_fire, NULL);

    schedule_entry_t e = make_sched_entry(100ULL, "Disabled");
    e.flags = 0; /* not enabled */
    scheduler_add(s, &e);

    scheduler_tick(s, 200ULL);
    TEST_ASSERT(fire_count_g == 0, "disabled entry does not fire");

    scheduler_destroy(s);
    TEST_PASS("scheduler skips disabled entries");
    return 0;
}

static int test_scheduler_clear(void) {
    printf("\n=== test_scheduler_clear ===\n");

    scheduler_t *s = scheduler_create(NULL, NULL);
    schedule_entry_t e1 = make_sched_entry(100ULL, "A");
    schedule_entry_t e2 = make_sched_entry(200ULL, "B");
    scheduler_add(s, &e1);
    scheduler_add(s, &e2);
    TEST_ASSERT(scheduler_count(s) == 2, "2 entries before clear");
    scheduler_clear(s);
    TEST_ASSERT(scheduler_count(s) == 0, "0 after clear");

    scheduler_destroy(s);
    TEST_PASS("scheduler clear");
    return 0;
}

static int test_scheduler_repeat(void) {
    printf("\n=== test_scheduler_repeat ===\n");

    fire_count_g = 0;
    scheduler_t *s = scheduler_create(on_fire, NULL);

    schedule_entry_t e = make_sched_entry(1000ULL, "Daily");
    e.flags = SCHED_FLAG_ENABLED | SCHED_FLAG_REPEAT;
    uint64_t id = scheduler_add(s, &e);

    /* Fire at t=1000 */
    scheduler_tick(s, 1000ULL);
    TEST_ASSERT(fire_count_g == 1, "first fire");
    TEST_ASSERT(scheduler_count(s) == 1, "repeat entry still active");

    /* Next fire should be at t = 1000 + 86400000000 */
    schedule_entry_t got;
    scheduler_get(s, id, &got);
    uint64_t expected_next = 1000ULL + 86400ULL * 1000000ULL;
    TEST_ASSERT(got.start_us == expected_next, "start_us advanced by 24h");

    /* Should NOT re-fire immediately */
    scheduler_tick(s, 1001ULL);
    TEST_ASSERT(fire_count_g == 1, "does not re-fire immediately");

    scheduler_destroy(s);
    TEST_PASS("scheduler repeat entry advances 24h");
    return 0;
}

/* ── schedule_store tests ────────────────────────────────────────── */

static int test_store_save_load(void) {
    printf("\n=== test_store_save_load ===\n");

    schedule_entry_t entries[3];
    entries[0] = make_sched_entry(1000ULL, "First");
    entries[1] = make_sched_entry(2000ULL, "Second");
    entries[2] = make_sched_entry(3000ULL, "Third");
    entries[0].flags = SCHED_FLAG_ENABLED;
    entries[1].flags = SCHED_FLAG_ENABLED | SCHED_FLAG_REPEAT;
    entries[2].flags = SCHED_FLAG_ENABLED;

    const char *path = "/tmp/test_schedule_store.bin";
    int rc = schedule_store_save(path, entries, 3);
    TEST_ASSERT(rc == 0, "save returns 0");

    schedule_entry_t loaded[8];
    size_t count = 0;
    rc = schedule_store_load(path, loaded, 8, &count);
    TEST_ASSERT(rc == 0, "load returns 0");
    TEST_ASSERT(count == 3, "loaded 3 entries");
    TEST_ASSERT(strcmp(loaded[0].title, "First")  == 0, "entry 0 title");
    TEST_ASSERT(strcmp(loaded[1].title, "Second") == 0, "entry 1 title");
    TEST_ASSERT(strcmp(loaded[2].title, "Third")  == 0, "entry 2 title");
    TEST_ASSERT(loaded[1].flags & SCHED_FLAG_REPEAT, "repeat flag preserved");

    remove(path);
    TEST_PASS("schedule store save/load");
    return 0;
}

static int test_store_load_missing(void) {
    printf("\n=== test_store_load_missing ===\n");

    schedule_entry_t buf[4];
    size_t count = 0;
    int rc = schedule_store_load("/tmp/no_such_schedule_file.bin",
                                  buf, 4, &count);
    TEST_ASSERT(rc == -1, "load missing file returns -1");

    TEST_PASS("schedule store: load missing file returns -1");
    return 0;
}

/* ── schedule_clock tests ────────────────────────────────────────── */

static int test_clock_now_monotonic(void) {
    printf("\n=== test_clock_now_monotonic ===\n");

    uint64_t t1 = schedule_clock_now_us();
    uint64_t t2 = schedule_clock_now_us();
    TEST_ASSERT(t1 > 0, "wall clock > 0");
    TEST_ASSERT(t2 >= t1, "wall clock is non-decreasing");

    uint64_t m1 = schedule_clock_mono_us();
    uint64_t m2 = schedule_clock_mono_us();
    TEST_ASSERT(m2 >= m1, "mono clock is non-decreasing");

    TEST_PASS("clock now/mono non-decreasing");
    return 0;
}

static int test_clock_format(void) {
    printf("\n=== test_clock_format ===\n");

    /* Use a known epoch: 2024-01-01 00:00:00 UTC = 1704067200 s */
    uint64_t ts = 1704067200ULL * 1000000ULL;
    char buf[32];
    char *r = schedule_clock_format(ts, buf, sizeof(buf));
    TEST_ASSERT(r != NULL, "format returns non-NULL");
    TEST_ASSERT(strncmp(buf, "2024-01-01", 10) == 0, "format YYYY-MM-DD correct");

    /* Buffer too small */
    r = schedule_clock_format(ts, buf, 5);
    TEST_ASSERT(r == NULL, "too-small buffer returns NULL");

    TEST_PASS("schedule_clock_format");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_entry_roundtrip();
    failures += test_entry_bad_magic();
    failures += test_entry_is_enabled();
    failures += test_entry_null_guards();

    failures += test_scheduler_create();
    failures += test_scheduler_add_remove();
    failures += test_scheduler_tick_fires();
    failures += test_scheduler_tick_disabled();
    failures += test_scheduler_clear();
    failures += test_scheduler_repeat();

    failures += test_store_save_load();
    failures += test_store_load_missing();

    failures += test_clock_now_monotonic();
    failures += test_clock_format();

    printf("\n");
    if (failures == 0)
        printf("ALL SCHEDULER TESTS PASSED\n");
    else
        printf("%d SCHEDULER TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
