/*
 * test_metadata.c — Unit tests for PHASE-49 Content Metadata Pipeline
 *
 * Tests stream_metadata (encode/decode/is_live), metadata_store
 * (set/get/delete/has/count/clear/foreach), and metadata_export
 * (JSON for both metadata and store).  No network or hardware required.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/metadata/stream_metadata.h"
#include "../../src/metadata/metadata_store.h"
#include "../../src/metadata/metadata_export.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── stream_metadata tests ───────────────────────────────────────── */

static int test_metadata_roundtrip(void) {
    printf("\n=== test_metadata_roundtrip ===\n");

    stream_metadata_t orig;
    memset(&orig, 0, sizeof(orig));
    orig.start_us     = 1700000000000000ULL;
    orig.duration_us  = 3600;
    orig.video_width  = 1920;
    orig.video_height = 1080;
    orig.video_fps    = 30;
    orig.flags        = METADATA_FLAG_LIVE | METADATA_FLAG_PUBLIC;
    snprintf(orig.title,       sizeof(orig.title),       "Test Stream");
    snprintf(orig.description, sizeof(orig.description), "A test stream");
    snprintf(orig.tags,        sizeof(orig.tags),        "test,live,hd");

    uint8_t buf[4096];
    int n = stream_metadata_encode(&orig, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "encode positive");

    stream_metadata_t decoded;
    int rc = stream_metadata_decode(buf, (size_t)n, &decoded);
    TEST_ASSERT(rc == 0, "decode ok");
    TEST_ASSERT(decoded.start_us     == orig.start_us,     "start_us preserved");
    TEST_ASSERT(decoded.video_width  == orig.video_width,  "width preserved");
    TEST_ASSERT(decoded.video_height == orig.video_height, "height preserved");
    TEST_ASSERT(decoded.video_fps    == orig.video_fps,    "fps preserved");
    TEST_ASSERT(decoded.flags        == orig.flags,        "flags preserved");
    TEST_ASSERT(strcmp(decoded.title, "Test Stream") == 0, "title preserved");
    TEST_ASSERT(strcmp(decoded.tags,  "test,live,hd") == 0, "tags preserved");

    TEST_PASS("stream_metadata encode/decode round-trip");
    return 0;
}

static int test_metadata_bad_magic(void) {
    printf("\n=== test_metadata_bad_magic ===\n");

    uint8_t buf[METADATA_FIXED_HDR_SZ + 16] = {0};
    stream_metadata_t m;
    TEST_ASSERT(stream_metadata_decode(buf, sizeof(buf), &m) == -1,
                "bad magic → -1");

    TEST_PASS("stream_metadata bad magic rejected");
    return 0;
}

static int test_metadata_is_live(void) {
    printf("\n=== test_metadata_is_live ===\n");

    stream_metadata_t m; memset(&m, 0, sizeof(m));
    m.flags = METADATA_FLAG_LIVE;
    TEST_ASSERT(stream_metadata_is_live(&m), "is_live true");

    m.flags = 0;
    TEST_ASSERT(!stream_metadata_is_live(&m), "is_live false");

    TEST_ASSERT(!stream_metadata_is_live(NULL), "is_live NULL false");

    TEST_PASS("stream_metadata is_live");
    return 0;
}

/* ── metadata_store tests ────────────────────────────────────────── */

static int test_store_set_get(void) {
    printf("\n=== test_store_set_get ===\n");

    metadata_store_t *s = metadata_store_create();
    TEST_ASSERT(s != NULL, "store created");
    TEST_ASSERT(metadata_store_count(s) == 0, "initial count 0");

    int rc = metadata_store_set(s, "song", "Hello");
    TEST_ASSERT(rc == 0, "set returns 0");
    TEST_ASSERT(metadata_store_count(s) == 1, "count 1");

    const char *val = metadata_store_get(s, "song");
    TEST_ASSERT(val != NULL, "get not null");
    TEST_ASSERT(strcmp(val, "Hello") == 0, "get value correct");

    /* Update existing key */
    rc = metadata_store_set(s, "song", "World");
    TEST_ASSERT(rc == 0, "update returns 0");
    TEST_ASSERT(metadata_store_count(s) == 1, "count still 1 after update");
    val = metadata_store_get(s, "song");
    TEST_ASSERT(strcmp(val, "World") == 0, "updated value");

    /* Non-existent key */
    TEST_ASSERT(metadata_store_get(s, "missing") == NULL, "missing key → NULL");

    metadata_store_destroy(s);
    TEST_PASS("metadata_store set/get");
    return 0;
}

static int test_store_delete(void) {
    printf("\n=== test_store_delete ===\n");

    metadata_store_t *s = metadata_store_create();
    metadata_store_set(s, "viewers", "42");

    TEST_ASSERT(metadata_store_has(s, "viewers"), "has key");
    int rc = metadata_store_delete(s, "viewers");
    TEST_ASSERT(rc == 0, "delete returns 0");
    TEST_ASSERT(!metadata_store_has(s, "viewers"), "key gone after delete");
    TEST_ASSERT(metadata_store_count(s) == 0, "count 0 after delete");

    rc = metadata_store_delete(s, "nonexistent");
    TEST_ASSERT(rc == -1, "delete nonexistent → -1");

    metadata_store_destroy(s);
    TEST_PASS("metadata_store delete");
    return 0;
}

static int test_store_clear(void) {
    printf("\n=== test_store_clear ===\n");

    metadata_store_t *s = metadata_store_create();
    metadata_store_set(s, "a", "1");
    metadata_store_set(s, "b", "2");
    TEST_ASSERT(metadata_store_count(s) == 2, "2 entries");

    metadata_store_clear(s);
    TEST_ASSERT(metadata_store_count(s) == 0, "0 after clear");
    TEST_ASSERT(!metadata_store_has(s, "a"), "a gone after clear");

    metadata_store_destroy(s);
    TEST_PASS("metadata_store clear");
    return 0;
}

static int foreach_count_cb(const char *key, const char *val, void *ud) {
    (void)key; (void)val;
    int *count = (int *)ud;
    (*count)++;
    return 0;
}

static int test_store_foreach(void) {
    printf("\n=== test_store_foreach ===\n");

    metadata_store_t *s = metadata_store_create();
    metadata_store_set(s, "x", "1");
    metadata_store_set(s, "y", "2");
    metadata_store_set(s, "z", "3");

    int count = 0;
    metadata_store_foreach(s, foreach_count_cb, &count);
    TEST_ASSERT(count == 3, "foreach visits all 3 entries");

    metadata_store_destroy(s);
    TEST_PASS("metadata_store foreach");
    return 0;
}

/* ── metadata_export tests ───────────────────────────────────────── */

static int test_export_metadata_json(void) {
    printf("\n=== test_export_metadata_json ===\n");

    stream_metadata_t m; memset(&m, 0, sizeof(m));
    m.start_us     = 100;
    m.video_width  = 1280;
    m.video_height = 720;
    m.video_fps    = 30;
    m.flags        = METADATA_FLAG_LIVE;
    snprintf(m.title, sizeof(m.title), "My Stream");

    char buf[4096];
    int n = metadata_export_json(&m, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "export JSON positive");
    TEST_ASSERT(strstr(buf, "\"live\":true") != NULL, "live flag in JSON");
    TEST_ASSERT(strstr(buf, "\"title\":\"My Stream\"") != NULL, "title in JSON");
    TEST_ASSERT(strstr(buf, "\"video_width\":1280") != NULL, "width in JSON");
    TEST_ASSERT(buf[0] == '{', "starts with {");
    TEST_ASSERT(buf[n-1] == '}', "ends with }");

    /* Buffer too small */
    n = metadata_export_json(&m, buf, 5);
    TEST_ASSERT(n == -1, "too-small buffer → -1");

    TEST_PASS("metadata_export JSON");
    return 0;
}

static int test_export_store_json(void) {
    printf("\n=== test_export_store_json ===\n");

    metadata_store_t *s = metadata_store_create();
    metadata_store_set(s, "song", "Test Title");
    metadata_store_set(s, "viewers", "99");

    char buf[4096];
    int n = metadata_store_export_json(s, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "store JSON positive");
    TEST_ASSERT(buf[0] == '{', "starts with {");
    TEST_ASSERT(buf[n-1] == '}', "ends with }");
    TEST_ASSERT(strstr(buf, "\"song\":\"Test Title\"") != NULL, "song in JSON");
    TEST_ASSERT(strstr(buf, "\"viewers\":\"99\"") != NULL, "viewers in JSON");

    metadata_store_destroy(s);
    TEST_PASS("metadata_store_export_json");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_metadata_roundtrip();
    failures += test_metadata_bad_magic();
    failures += test_metadata_is_live();

    failures += test_store_set_get();
    failures += test_store_delete();
    failures += test_store_clear();
    failures += test_store_foreach();

    failures += test_export_metadata_json();
    failures += test_export_store_json();

    printf("\n");
    if (failures == 0)
        printf("ALL METADATA TESTS PASSED\n");
    else
        printf("%d METADATA TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
