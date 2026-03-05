/*
 * test_tagging.c — Unit tests for PHASE-73 Stream Tag Store
 *
 * Tests tag_entry (init/empty_key), tag_store (set/get/remove/clear/
 * count/overwrite/full-guard/foreach), and tag_serial
 * (write/read round-trip).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/tagging/tag_entry.h"
#include "../../src/tagging/tag_store.h"
#include "../../src/tagging/tag_serial.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── tag_entry ───────────────────────────────────────────────────── */

static int test_entry_init(void) {
    printf("\n=== test_entry_init ===\n");

    tag_entry_t e;
    TEST_ASSERT(tag_entry_init(&e, "title", "My Stream") == 0, "init ok");
    TEST_ASSERT(strcmp(e.key, "title") == 0, "key");
    TEST_ASSERT(strcmp(e.value, "My Stream") == 0, "value");
    TEST_ASSERT(e.in_use, "in_use");

    TEST_ASSERT(tag_entry_init(NULL, "k", "v") == -1, "NULL → -1");
    TEST_ASSERT(tag_entry_init(&e, "", "v")    == -1, "empty key → -1");
    TEST_ASSERT(tag_entry_init(&e, NULL, "v")  == -1, "NULL key → -1");

    TEST_PASS("tag_entry init / null/empty guard");
    return 0;
}

/* ── tag_store ───────────────────────────────────────────────────── */

static int test_store_set_get(void) {
    printf("\n=== test_store_set_get ===\n");

    tag_store_t *s = tag_store_create();
    TEST_ASSERT(s != NULL, "created");
    TEST_ASSERT(tag_store_count(s) == 0, "initially 0");

    TEST_ASSERT(tag_store_set(s, "game", "FPS") == 0, "set game");
    TEST_ASSERT(tag_store_set(s, "title", "Live") == 0, "set title");
    TEST_ASSERT(tag_store_count(s) == 2, "count = 2");

    TEST_ASSERT(strcmp(tag_store_get(s, "game"), "FPS") == 0, "get game");
    TEST_ASSERT(tag_store_get(s, "missing") == NULL, "missing → NULL");

    /* Overwrite */
    TEST_ASSERT(tag_store_set(s, "game", "RPG") == 0, "overwrite ok");
    TEST_ASSERT(strcmp(tag_store_get(s, "game"), "RPG") == 0, "overwrite value");
    TEST_ASSERT(tag_store_count(s) == 2, "count unchanged after overwrite");

    /* Remove */
    TEST_ASSERT(tag_store_remove(s, "game") == 0, "remove ok");
    TEST_ASSERT(tag_store_count(s) == 1, "count = 1 after remove");
    TEST_ASSERT(tag_store_remove(s, "game") == -1, "remove missing → -1");

    tag_store_destroy(s);
    TEST_PASS("tag_store set/get/overwrite/remove/count");
    return 0;
}

static int test_store_clear(void) {
    printf("\n=== test_store_clear ===\n");

    tag_store_t *s = tag_store_create();
    tag_store_set(s, "a", "1");
    tag_store_set(s, "b", "2");
    tag_store_clear(s);
    TEST_ASSERT(tag_store_count(s) == 0, "clear → count = 0");
    TEST_ASSERT(tag_store_get(s, "a") == NULL, "cleared tag not found");

    tag_store_destroy(s);
    TEST_PASS("tag_store clear");
    return 0;
}

static void count_cb(const tag_entry_t *e, void *user) {
    (void)e;
    (*(int *)user)++;
}

static int test_store_foreach(void) {
    printf("\n=== test_store_foreach ===\n");

    tag_store_t *s = tag_store_create();
    tag_store_set(s, "x", "1");
    tag_store_set(s, "y", "2");
    tag_store_set(s, "z", "3");

    int count = 0;
    tag_store_foreach(s, count_cb, &count);
    TEST_ASSERT(count == 3, "foreach visits 3 tags");

    tag_store_destroy(s);
    TEST_PASS("tag_store foreach");
    return 0;
}

/* ── tag_serial ──────────────────────────────────────────────────── */

static int test_serial_round_trip(void) {
    printf("\n=== test_serial_round_trip ===\n");

    tag_store_t *src = tag_store_create();
    tag_store_set(src, "title", "My Stream");
    tag_store_set(src, "game",  "FPS");
    tag_store_set(src, "lang",  "en");

    char buf[512];
    int written = tag_serial_write(src, buf, sizeof(buf));
    TEST_ASSERT(written > 0, "write produced output");
    /* Should contain all three keys */
    TEST_ASSERT(strstr(buf, "title=My Stream") != NULL, "title in output");
    TEST_ASSERT(strstr(buf, "game=FPS")         != NULL, "game in output");
    TEST_ASSERT(strstr(buf, "lang=en")           != NULL, "lang in output");

    /* Parse back into a fresh store */
    tag_store_t *dst = tag_store_create();
    int parsed = tag_serial_read(dst, buf);
    TEST_ASSERT(parsed == 3, "parsed 3 tags");
    TEST_ASSERT(strcmp(tag_store_get(dst, "title"), "My Stream") == 0, "title round-trip");
    TEST_ASSERT(strcmp(tag_store_get(dst, "game"),  "FPS")       == 0, "game round-trip");
    TEST_ASSERT(strcmp(tag_store_get(dst, "lang"),  "en")        == 0, "lang round-trip");

    tag_store_destroy(src);
    tag_store_destroy(dst);
    TEST_PASS("tag_serial write / read round-trip");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_entry_init();
    failures += test_store_set_get();
    failures += test_store_clear();
    failures += test_store_foreach();
    failures += test_serial_round_trip();

    printf("\n");
    if (failures == 0) printf("ALL TAGGING TESTS PASSED\n");
    else               printf("%d TAGGING TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
