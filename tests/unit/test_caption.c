/*
 * test_caption.c — Unit tests for PHASE-42 Closed-Caption & Subtitle System
 *
 * Tests caption_event (encode/decode/is_active), caption_buffer
 * (push/query/expire/clear), and caption_renderer (draw no-crash,
 * pixel modification). No audio/video hardware required.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/caption/caption_event.h"
#include "../../src/caption/caption_buffer.h"
#include "../../src/caption/caption_renderer.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── Helpers ─────────────────────────────────────────────────────── */

static caption_event_t make_event(uint64_t pts_us, uint32_t dur_us,
                                   const char *text, uint8_t row) {
    caption_event_t e;
    memset(&e, 0, sizeof(e));
    e.pts_us      = pts_us;
    e.duration_us = dur_us;
    e.row         = row;
    e.flags       = CAPTION_FLAG_BOTTOM;
    e.text_len    = (uint16_t)strlen(text);
    snprintf(e.text, sizeof(e.text), "%s", text);
    return e;
}

/* ── caption_event tests ─────────────────────────────────────────── */

static int test_event_encode_decode(void) {
    printf("\n=== test_event_encode_decode ===\n");

    caption_event_t orig = make_event(1000000ULL, 3000000U,
                                       "Hello, World!", 0);
    uint8_t buf[512];
    int n = caption_event_encode(&orig, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "encode returns positive size");
    TEST_ASSERT((size_t)n == caption_event_encoded_size(&orig),
                "encoded size matches predicted");

    caption_event_t decoded;
    int rc = caption_event_decode(buf, (size_t)n, &decoded);
    TEST_ASSERT(rc == 0, "decode succeeds");
    TEST_ASSERT(decoded.pts_us      == 1000000ULL, "pts_us preserved");
    TEST_ASSERT(decoded.duration_us == 3000000U,   "duration preserved");
    TEST_ASSERT(decoded.text_len    == orig.text_len, "text_len preserved");
    TEST_ASSERT(strcmp(decoded.text, "Hello, World!") == 0, "text preserved");
    TEST_ASSERT(decoded.row         == 0,           "row preserved");

    TEST_PASS("caption event encode/decode round-trip");
    return 0;
}

static int test_event_bad_magic(void) {
    printf("\n=== test_event_bad_magic ===\n");

    uint8_t buf[32] = {0xFF, 0xFF, 0xFF, 0xFF};
    caption_event_t e;
    int rc = caption_event_decode(buf, sizeof(buf), &e);
    TEST_ASSERT(rc == -1, "bad magic returns -1");

    TEST_PASS("caption event rejects bad magic");
    return 0;
}

static int test_event_is_active(void) {
    printf("\n=== test_event_is_active ===\n");

    caption_event_t e = make_event(5000000ULL, 2000000U, "test", 0);

    TEST_ASSERT(!caption_event_is_active(&e, 4999999ULL), "before PTS: not active");
    TEST_ASSERT( caption_event_is_active(&e, 5000000ULL), "at PTS: active");
    TEST_ASSERT( caption_event_is_active(&e, 6000000ULL), "during: active");
    TEST_ASSERT(!caption_event_is_active(&e, 7000000ULL), "after end: not active");
    TEST_ASSERT(!caption_event_is_active(NULL, 5000000ULL), "NULL event: not active");

    TEST_PASS("caption event is_active timing");
    return 0;
}

static int test_event_null_guards(void) {
    printf("\n=== test_event_null_guards ===\n");

    caption_event_t e = make_event(0, 1000000U, "x", 0);
    uint8_t buf[32];
    TEST_ASSERT(caption_event_encode(NULL, buf, sizeof(buf)) == -1,
                "encode NULL event returns -1");
    TEST_ASSERT(caption_event_encode(&e, NULL, 0) == -1,
                "encode NULL buf returns -1");

    caption_event_t out;
    TEST_ASSERT(caption_event_decode(NULL, 0, &out) == -1,
                "decode NULL buf returns -1");

    TEST_PASS("caption event NULL guards");
    return 0;
}

/* ── caption_buffer tests ────────────────────────────────────────── */

static int test_buffer_create(void) {
    printf("\n=== test_buffer_create ===\n");

    caption_buffer_t *b = caption_buffer_create();
    TEST_ASSERT(b != NULL, "buffer created");
    TEST_ASSERT(caption_buffer_count(b) == 0, "initial count 0");
    caption_buffer_destroy(b);
    caption_buffer_destroy(NULL); /* must not crash */
    TEST_PASS("caption buffer create/destroy");
    return 0;
}

static int test_buffer_push_query(void) {
    printf("\n=== test_buffer_push_query ===\n");

    caption_buffer_t *b = caption_buffer_create();
    TEST_ASSERT(b != NULL, "buffer created");

    /* Add three events at different times */
    caption_event_t e1 = make_event(1000000ULL, 2000000U, "First",  0);
    caption_event_t e2 = make_event(2000000ULL, 2000000U, "Second", 0);
    caption_event_t e3 = make_event(5000000ULL, 2000000U, "Third",  0);

    caption_buffer_push(b, &e1);
    caption_buffer_push(b, &e2);
    caption_buffer_push(b, &e3);
    TEST_ASSERT(caption_buffer_count(b) == 3, "3 events in buffer");

    /* Query at t=1.5s: only e1 active */
    caption_event_t out[8];
    int n = caption_buffer_query(b, 1500000ULL, out, 8);
    TEST_ASSERT(n == 1, "1 event active at 1.5s");
    TEST_ASSERT(strcmp(out[0].text, "First") == 0, "correct event");

    /* Query at t=2.5s: e1 and e2 both active */
    n = caption_buffer_query(b, 2500000ULL, out, 8);
    TEST_ASSERT(n == 2, "2 events active at 2.5s");

    caption_buffer_destroy(b);
    TEST_PASS("caption buffer push/query");
    return 0;
}

static int test_buffer_expire(void) {
    printf("\n=== test_buffer_expire ===\n");

    caption_buffer_t *b = caption_buffer_create();
    caption_event_t ea = make_event(0ULL, 1000000U, "A", 0);
    caption_event_t eb = make_event(0ULL, 2000000U, "B", 0);
    caption_event_t ec = make_event(5000000ULL, 1000000U, "C", 0);
    caption_buffer_push(b, &ea);
    caption_buffer_push(b, &eb);
    caption_buffer_push(b, &ec);
    TEST_ASSERT(caption_buffer_count(b) == 3, "3 events");

    /* Expire at t=3s: A and B ended, C not yet started */
    int removed = caption_buffer_expire(b, 3000000ULL);
    TEST_ASSERT(removed == 2, "2 events expired");
    TEST_ASSERT(caption_buffer_count(b) == 1, "1 event remains");

    caption_buffer_destroy(b);
    TEST_PASS("caption buffer expire");
    return 0;
}

static int test_buffer_clear(void) {
    printf("\n=== test_buffer_clear ===\n");

    caption_buffer_t *b = caption_buffer_create();
    caption_event_t ex = make_event(0, 1000000U, "x", 0);
    caption_event_t ey = make_event(0, 1000000U, "y", 0);
    caption_buffer_push(b, &ex);
    caption_buffer_push(b, &ey);
    TEST_ASSERT(caption_buffer_count(b) == 2, "2 events before clear");

    caption_buffer_clear(b);
    TEST_ASSERT(caption_buffer_count(b) == 0, "0 after clear");

    caption_buffer_destroy(b);
    TEST_PASS("caption buffer clear");
    return 0;
}

static int test_buffer_sorted_insert(void) {
    printf("\n=== test_buffer_sorted_insert ===\n");

    caption_buffer_t *b = caption_buffer_create();

    /* Insert out of PTS order */
    caption_event_t e3 = make_event(3000000ULL, 1000000U, "3rd", 0);
    caption_event_t e1 = make_event(1000000ULL, 1000000U, "1st", 0);
    caption_event_t e2 = make_event(2000000ULL, 1000000U, "2nd", 0);

    caption_buffer_push(b, &e3);
    caption_buffer_push(b, &e1);
    caption_buffer_push(b, &e2);

    /* Query at t=1s: only 1st active */
    caption_event_t out[4];
    int n = caption_buffer_query(b, 1500000ULL, out, 4);
    TEST_ASSERT(n == 1, "1 event at 1.5s");
    TEST_ASSERT(strcmp(out[0].text, "1st") == 0, "1st event at 1.5s");

    caption_buffer_destroy(b);
    TEST_PASS("caption buffer sorted insertion");
    return 0;
}

/* ── caption_renderer tests ──────────────────────────────────────── */

static int test_renderer_create(void) {
    printf("\n=== test_renderer_create ===\n");

    caption_renderer_t *r = caption_renderer_create(NULL);
    TEST_ASSERT(r != NULL, "renderer created with defaults");
    caption_renderer_destroy(r);
    caption_renderer_destroy(NULL); /* must not crash */

    caption_renderer_config_t cfg = { 0xBB000000, 0xFFFFFFFF, 2, 8 };
    r = caption_renderer_create(&cfg);
    TEST_ASSERT(r != NULL, "renderer created with config");
    caption_renderer_destroy(r);

    TEST_PASS("caption renderer create/destroy");
    return 0;
}

static int test_renderer_draw_active(void) {
    printf("\n=== test_renderer_draw_active ===\n");

    caption_renderer_t *r = caption_renderer_create(NULL);
    TEST_ASSERT(r != NULL, "renderer created");

    const int W = 320, H = 240;
    uint8_t *pixels = calloc((size_t)(W * H * 4), 1);
    TEST_ASSERT(pixels != NULL, "pixel buffer allocated");

    caption_event_t e = make_event(0ULL, 5000000U, "Hello", 0);
    int n = caption_renderer_draw(r, pixels, W, H, W * 4, &e, 1, 1000000ULL);
    TEST_ASSERT(n == 1, "1 caption rendered");

    /* At least one pixel should have been modified */
    bool modified = false;
    for (int i = 0; i < W * H * 4; i++) {
        if (pixels[i] != 0) { modified = true; break; }
    }
    TEST_ASSERT(modified, "pixels written by renderer");

    free(pixels);
    caption_renderer_destroy(r);
    TEST_PASS("caption renderer draws active caption");
    return 0;
}

static int test_renderer_draw_inactive(void) {
    printf("\n=== test_renderer_draw_inactive ===\n");

    caption_renderer_t *r = caption_renderer_create(NULL);
    const int W = 128, H = 72;
    uint8_t *pixels = calloc((size_t)(W * H * 4), 1);

    /* Event not yet visible at now_us=0 */
    caption_event_t e = make_event(10000000ULL, 2000000U, "Future", 0);
    int n = caption_renderer_draw(r, pixels, W, H, W * 4, &e, 1, 0ULL);
    TEST_ASSERT(n == 0, "inactive caption not rendered");

    free(pixels);
    caption_renderer_destroy(r);
    TEST_PASS("caption renderer skips inactive captions");
    return 0;
}

static int test_renderer_null_guards(void) {
    printf("\n=== test_renderer_null_guards ===\n");

    uint8_t buf[4] = {0};
    caption_event_t e = make_event(0, 1000000U, "x", 0);
    int n = caption_renderer_draw(NULL, buf, 1, 1, 4, &e, 1, 0ULL);
    TEST_ASSERT(n == 0, "NULL renderer returns 0");

    caption_renderer_t *r = caption_renderer_create(NULL);
    n = caption_renderer_draw(r, NULL, 1, 1, 4, &e, 1, 0ULL);
    TEST_ASSERT(n == 0, "NULL pixels returns 0");
    caption_renderer_destroy(r);

    TEST_PASS("caption renderer NULL guards");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_event_encode_decode();
    failures += test_event_bad_magic();
    failures += test_event_is_active();
    failures += test_event_null_guards();

    failures += test_buffer_create();
    failures += test_buffer_push_query();
    failures += test_buffer_expire();
    failures += test_buffer_clear();
    failures += test_buffer_sorted_insert();

    failures += test_renderer_create();
    failures += test_renderer_draw_active();
    failures += test_renderer_draw_inactive();
    failures += test_renderer_null_guards();

    printf("\n");
    if (failures == 0)
        printf("ALL CAPTION TESTS PASSED\n");
    else
        printf("%d CAPTION TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
