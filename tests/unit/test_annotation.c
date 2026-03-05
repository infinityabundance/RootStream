/*
 * test_annotation.c — Unit tests for PHASE-38 Collaboration & Annotation
 *
 * Tests annotation_protocol (encode/decode round-trip), annotation_renderer
 * state management, and pointer_sync tracking.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/collab/annotation_protocol.h"
#include "../../src/collab/annotation_renderer.h"
#include "../../src/collab/pointer_sync.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── annotation_protocol tests ───────────────────────────────────── */

static int test_protocol_draw_begin_roundtrip(void) {
    printf("\n=== test_protocol_draw_begin_roundtrip ===\n");

    annotation_event_t orig;
    memset(&orig, 0, sizeof(orig));
    orig.type                   = ANNOT_DRAW_BEGIN;
    orig.seq                    = 42;
    orig.timestamp_us           = 1234567890ULL;
    orig.draw_begin.pos.x       = 0.3f;
    orig.draw_begin.pos.y       = 0.7f;
    orig.draw_begin.color       = 0xFF0000FFu; /* Opaque blue */
    orig.draw_begin.width       = 4.0f;
    orig.draw_begin.stroke_id   = 99;

    uint8_t buf[256];
    int encoded = annotation_encode(&orig, buf, sizeof(buf));
    TEST_ASSERT(encoded > 0, "encode returns positive size");
    TEST_ASSERT((size_t)encoded == annotation_encoded_size(&orig),
                "encoded size matches predicted size");

    annotation_event_t decoded;
    int rc = annotation_decode(buf, (size_t)encoded, &decoded);
    TEST_ASSERT(rc == 0, "decode succeeds");
    TEST_ASSERT(decoded.type            == ANNOT_DRAW_BEGIN, "type preserved");
    TEST_ASSERT(decoded.seq             == 42,               "seq preserved");
    TEST_ASSERT(decoded.timestamp_us    == 1234567890ULL,    "timestamp preserved");
    TEST_ASSERT(decoded.draw_begin.stroke_id == 99,          "stroke_id preserved");
    TEST_ASSERT(fabsf(decoded.draw_begin.pos.x - 0.3f) < 1e-5f, "pos.x preserved");
    TEST_ASSERT(fabsf(decoded.draw_begin.pos.y - 0.7f) < 1e-5f, "pos.y preserved");
    TEST_ASSERT(decoded.draw_begin.color == 0xFF0000FFu,     "color preserved");

    TEST_PASS("draw_begin encode/decode round-trip");
    return 0;
}

static int test_protocol_draw_point_roundtrip(void) {
    printf("\n=== test_protocol_draw_point_roundtrip ===\n");

    annotation_event_t orig = {
        .type                  = ANNOT_DRAW_POINT,
        .seq                   = 1,
        .timestamp_us          = 100ULL,
        .draw_point            = { .pos = {0.5f, 0.5f}, .stroke_id = 7 }
    };

    uint8_t buf[128];
    int encoded = annotation_encode(&orig, buf, sizeof(buf));
    TEST_ASSERT(encoded > 0, "encode succeeds");

    annotation_event_t decoded;
    TEST_ASSERT(annotation_decode(buf, (size_t)encoded, &decoded) == 0,
                "decode succeeds");
    TEST_ASSERT(decoded.draw_point.stroke_id == 7, "stroke_id preserved");

    TEST_PASS("draw_point round-trip");
    return 0;
}

static int test_protocol_text_roundtrip(void) {
    printf("\n=== test_protocol_text_roundtrip ===\n");

    annotation_event_t orig;
    memset(&orig, 0, sizeof(orig));
    orig.type            = ANNOT_TEXT;
    orig.seq             = 5;
    orig.timestamp_us    = 500ULL;
    orig.text.pos.x      = 0.1f;
    orig.text.pos.y      = 0.2f;
    orig.text.color      = 0xFFFF0000u; /* Opaque red */
    orig.text.font_size  = 24.0f;
    const char *msg = "Hello, World!";
    orig.text.text_len = (uint16_t)strlen(msg);
    memcpy(orig.text.text, msg, orig.text.text_len);

    uint8_t buf[512];
    int encoded = annotation_encode(&orig, buf, sizeof(buf));
    TEST_ASSERT(encoded > 0, "text encode succeeds");

    annotation_event_t decoded;
    TEST_ASSERT(annotation_decode(buf, (size_t)encoded, &decoded) == 0,
                "text decode succeeds");
    TEST_ASSERT(decoded.text.text_len == orig.text.text_len,
                "text_len preserved");
    TEST_ASSERT(memcmp(decoded.text.text, msg, orig.text.text_len) == 0,
                "text content preserved");
    TEST_ASSERT(fabsf(decoded.text.font_size - 24.0f) < 1e-4f,
                "font_size preserved");

    TEST_PASS("text annotation round-trip");
    return 0;
}

static int test_protocol_clear_all_roundtrip(void) {
    printf("\n=== test_protocol_clear_all_roundtrip ===\n");

    annotation_event_t orig = {
        .type = ANNOT_CLEAR_ALL, .seq = 0, .timestamp_us = 0
    };

    uint8_t buf[32];
    int encoded = annotation_encode(&orig, buf, sizeof(buf));
    TEST_ASSERT(encoded == ANNOTATION_HDR_SIZE, "clear_all is header-only");

    annotation_event_t decoded;
    TEST_ASSERT(annotation_decode(buf, (size_t)encoded, &decoded) == 0,
                "decode succeeds");
    TEST_ASSERT(decoded.type == ANNOT_CLEAR_ALL, "type preserved");

    TEST_PASS("clear_all round-trip");
    return 0;
}

static int test_protocol_bad_magic(void) {
    printf("\n=== test_protocol_bad_magic ===\n");

    uint8_t buf[32] = {0xFF, 0xFF, 1, 1}; /* bad magic */
    annotation_event_t decoded;
    int rc = annotation_decode(buf, sizeof(buf), &decoded);
    TEST_ASSERT(rc == -1, "bad magic returns -1");

    TEST_PASS("protocol rejects bad magic");
    return 0;
}

static int test_protocol_buffer_too_small(void) {
    printf("\n=== test_protocol_buffer_too_small ===\n");

    annotation_event_t orig = {
        .type = ANNOT_DRAW_BEGIN, .seq = 1, .timestamp_us = 0
    };

    uint8_t buf[4]; /* Far too small */
    int rc = annotation_encode(&orig, buf, sizeof(buf));
    TEST_ASSERT(rc == -1, "encode too-small buffer returns -1");

    TEST_PASS("encode rejects too-small buffer");
    return 0;
}

/* ── annotation_renderer tests ───────────────────────────────────── */

static int test_renderer_create_destroy(void) {
    printf("\n=== test_renderer_create_destroy ===\n");

    annotation_renderer_t *r = annotation_renderer_create();
    TEST_ASSERT(r != NULL, "renderer created");
    TEST_ASSERT(annotation_renderer_stroke_count(r) == 0,
                "initial stroke count == 0");

    annotation_renderer_destroy(r);
    annotation_renderer_destroy(NULL); /* must not crash */
    TEST_PASS("annotation renderer create/destroy");
    return 0;
}

static int test_renderer_strokes(void) {
    printf("\n=== test_renderer_strokes ===\n");

    annotation_renderer_t *r = annotation_renderer_create();
    TEST_ASSERT(r != NULL, "renderer created");

    /* Begin stroke */
    annotation_event_t e = {
        .type = ANNOT_DRAW_BEGIN,
        .draw_begin = { .pos = {0.1f, 0.1f}, .color = 0xFF0000FFu,
                        .width = 3.0f, .stroke_id = 1 }
    };
    annotation_renderer_apply_event(r, &e);
    TEST_ASSERT(annotation_renderer_stroke_count(r) == 1, "1 stroke after begin");

    /* Add points */
    for (int i = 1; i < 5; i++) {
        annotation_event_t pe = {
            .type = ANNOT_DRAW_POINT,
            .draw_point = { .pos = {0.1f + i*0.05f, 0.2f}, .stroke_id = 1 }
        };
        annotation_renderer_apply_event(r, &pe);
    }
    TEST_ASSERT(annotation_renderer_stroke_count(r) == 1,
                "still 1 stroke after points");

    /* End stroke */
    annotation_event_t end_e = {
        .type = ANNOT_DRAW_END,
        .draw_end = { .stroke_id = 1 }
    };
    annotation_renderer_apply_event(r, &end_e);
    TEST_ASSERT(annotation_renderer_stroke_count(r) == 1,
                "still 1 stroke after end");

    /* Second stroke */
    e.draw_begin.stroke_id = 2;
    annotation_renderer_apply_event(r, &e);
    TEST_ASSERT(annotation_renderer_stroke_count(r) == 2,
                "2 strokes after second begin");

    /* Clear */
    annotation_event_t clr = { .type = ANNOT_CLEAR_ALL };
    annotation_renderer_apply_event(r, &clr);
    TEST_ASSERT(annotation_renderer_stroke_count(r) == 0,
                "0 strokes after clear");

    annotation_renderer_destroy(r);
    TEST_PASS("annotation renderer stroke add/clear");
    return 0;
}

static int test_renderer_erase(void) {
    printf("\n=== test_renderer_erase ===\n");

    annotation_renderer_t *r = annotation_renderer_create();
    TEST_ASSERT(r != NULL, "renderer created");

    /* Place two strokes */
    for (uint32_t sid = 1; sid <= 2; sid++) {
        annotation_event_t e = {
            .type = ANNOT_DRAW_BEGIN,
            .draw_begin = {
                .pos = { sid == 1 ? 0.1f : 0.9f, 0.5f },
                .color = 0xFFFF0000u,
                .width = 3.0f,
                .stroke_id = sid
            }
        };
        annotation_renderer_apply_event(r, &e);
    }
    TEST_ASSERT(annotation_renderer_stroke_count(r) == 2,
                "2 strokes before erase");

    /* Erase region around first stroke */
    annotation_event_t ee = {
        .type = ANNOT_ERASE,
        .erase = { .center = {0.1f, 0.5f}, .radius = 0.1f }
    };
    annotation_renderer_apply_event(r, &ee);
    TEST_ASSERT(annotation_renderer_stroke_count(r) == 1,
                "1 stroke after targeted erase");

    annotation_renderer_destroy(r);
    TEST_PASS("annotation renderer erase");
    return 0;
}

static int test_renderer_composite_no_crash(void) {
    printf("\n=== test_renderer_composite_no_crash ===\n");

    annotation_renderer_t *r = annotation_renderer_create();

    /* Add a stroke and composite onto a small RGBA buffer */
    annotation_event_t e = {
        .type = ANNOT_DRAW_BEGIN,
        .draw_begin = { .pos = {0.5f, 0.5f}, .color = 0x80FF0000u,
                        .width = 5.0f, .stroke_id = 1 }
    };
    annotation_renderer_apply_event(r, &e);

    uint8_t pixels[64 * 64 * 4];
    memset(pixels, 0, sizeof(pixels));
    annotation_renderer_composite(r, pixels, 64, 64, 64 * 4);

    /* Just checking it didn't crash; verify a pixel near center was touched */
    bool modified = false;
    for (size_t i = 0; i < sizeof(pixels); i++) {
        if (pixels[i] != 0) { modified = true; break; }
    }
    TEST_ASSERT(modified, "composite modified at least one pixel");

    annotation_renderer_destroy(r);
    TEST_PASS("annotation renderer composite (no crash, pixels written)");
    return 0;
}

/* ── pointer_sync tests ──────────────────────────────────────────── */

static int test_pointer_sync_create(void) {
    printf("\n=== test_pointer_sync_create ===\n");

    pointer_sync_t *ps = pointer_sync_create(0);
    TEST_ASSERT(ps != NULL, "pointer sync created with default timeout");

    pointer_sync_destroy(ps);
    pointer_sync_destroy(NULL); /* must not crash */
    TEST_PASS("pointer_sync create/destroy");
    return 0;
}

static int test_pointer_sync_update_get(void) {
    printf("\n=== test_pointer_sync_update_get ===\n");

    pointer_sync_t *ps = pointer_sync_create(5000000ULL); /* 5 s */
    TEST_ASSERT(ps != NULL, "pointer sync created");

    annotation_event_t e = {
        .type = ANNOT_POINTER_MOVE,
        .timestamp_us = 1000ULL,
        .pointer_move = { .pos = {0.4f, 0.6f}, .peer_id = 42 }
    };
    pointer_sync_update(ps, &e);

    remote_pointer_t rp;
    int rc = pointer_sync_get(ps, 42, &rp);
    TEST_ASSERT(rc == 0, "get peer 42 succeeds");
    TEST_ASSERT(rp.visible, "peer is visible");
    TEST_ASSERT(rp.peer_id == 42, "peer_id correct");
    TEST_ASSERT(fabsf(rp.pos.x - 0.4f) < 1e-5f, "pos.x correct");
    TEST_ASSERT(fabsf(rp.pos.y - 0.6f) < 1e-5f, "pos.y correct");

    /* Unknown peer */
    rc = pointer_sync_get(ps, 99, &rp);
    TEST_ASSERT(rc == -1, "get unknown peer returns -1");

    pointer_sync_destroy(ps);
    TEST_PASS("pointer_sync update/get");
    return 0;
}

static int test_pointer_sync_hide(void) {
    printf("\n=== test_pointer_sync_hide ===\n");

    pointer_sync_t *ps = pointer_sync_create(0);

    annotation_event_t mv = {
        .type = ANNOT_POINTER_MOVE,
        .timestamp_us = 1000ULL,
        .pointer_move = { .pos = {0.5f, 0.5f}, .peer_id = 1 }
    };
    pointer_sync_update(ps, &mv);

    annotation_event_t hide = { .type = ANNOT_POINTER_HIDE };
    pointer_sync_update(ps, &hide);

    remote_pointer_t rp;
    pointer_sync_get(ps, 1, &rp);
    TEST_ASSERT(!rp.visible, "peer hidden after POINTER_HIDE");

    /* get_all should return 0 visible */
    remote_pointer_t all[8];
    int n = pointer_sync_get_all(ps, all, 8);
    TEST_ASSERT(n == 0, "get_all returns 0 after hide");

    pointer_sync_destroy(ps);
    TEST_PASS("pointer_sync hide");
    return 0;
}

static int test_pointer_sync_expire(void) {
    printf("\n=== test_pointer_sync_expire ===\n");

    pointer_sync_t *ps = pointer_sync_create(1000ULL); /* 1 ms timeout */

    annotation_event_t mv = {
        .type = ANNOT_POINTER_MOVE,
        .timestamp_us = 0ULL,
        .pointer_move = { .pos = {0.5f, 0.5f}, .peer_id = 7 }
    };
    pointer_sync_update(ps, &mv);

    remote_pointer_t rp;
    pointer_sync_get(ps, 7, &rp);
    TEST_ASSERT(rp.visible, "peer visible before expire");

    /* Expire at t = 2000 µs >> 1000 µs timeout */
    pointer_sync_expire(ps, 2000ULL);

    pointer_sync_get(ps, 7, &rp);
    TEST_ASSERT(!rp.visible, "peer invisible after expire");

    pointer_sync_destroy(ps);
    TEST_PASS("pointer_sync expire");
    return 0;
}

static int test_pointer_sync_get_all(void) {
    printf("\n=== test_pointer_sync_get_all ===\n");

    pointer_sync_t *ps = pointer_sync_create(0);

    for (uint32_t pid = 1; pid <= 4; pid++) {
        annotation_event_t mv = {
            .type = ANNOT_POINTER_MOVE,
            .timestamp_us = 1000ULL,
            .pointer_move = { .pos = {0.1f * pid, 0.5f}, .peer_id = pid }
        };
        pointer_sync_update(ps, &mv);
    }

    remote_pointer_t all[8];
    int n = pointer_sync_get_all(ps, all, 8);
    TEST_ASSERT(n == 4, "get_all returns 4 visible peers");

    pointer_sync_destroy(ps);
    TEST_PASS("pointer_sync get_all multiple peers");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    /* Protocol */
    failures += test_protocol_draw_begin_roundtrip();
    failures += test_protocol_draw_point_roundtrip();
    failures += test_protocol_text_roundtrip();
    failures += test_protocol_clear_all_roundtrip();
    failures += test_protocol_bad_magic();
    failures += test_protocol_buffer_too_small();

    /* Renderer */
    failures += test_renderer_create_destroy();
    failures += test_renderer_strokes();
    failures += test_renderer_erase();
    failures += test_renderer_composite_no_crash();

    /* Pointer sync */
    failures += test_pointer_sync_create();
    failures += test_pointer_sync_update_get();
    failures += test_pointer_sync_hide();
    failures += test_pointer_sync_expire();
    failures += test_pointer_sync_get_all();

    printf("\n");
    if (failures == 0) {
        printf("ALL ANNOTATION TESTS PASSED\n");
    } else {
        printf("%d ANNOTATION TEST(S) FAILED\n", failures);
    }
    return failures ? 1 : 0;
}
