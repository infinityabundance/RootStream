/*
 * test_watermark.c — Unit tests for PHASE-47 Stream Watermarking
 *
 * Tests watermark_payload (encode/decode/to_bits/from_bits),
 * watermark_lsb (embed/extract round-trip, invisibility),
 * watermark_dct (embed/extract round-trip on synthetic frame),
 * and watermark_strength (mode selection, mode name).
 *
 * No video hardware or network required.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/watermark/watermark_payload.h"
#include "../../src/watermark/watermark_lsb.h"
#include "../../src/watermark/watermark_dct.h"
#include "../../src/watermark/watermark_strength.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── watermark_payload tests ─────────────────────────────────────── */

static int test_payload_roundtrip(void) {
    printf("\n=== test_payload_roundtrip ===\n");

    watermark_payload_t orig;
    memset(&orig, 0, sizeof(orig));
    orig.viewer_id    = 0xDEADBEEFCAFEBABEULL;
    orig.session_id   = 0x0102030405060708ULL;
    orig.timestamp_us = 1700000000000000ULL;
    orig.payload_bits = 64;

    uint8_t buf[WATERMARK_HDR_SIZE + WATERMARK_MAX_DATA_BYTES + 8];
    int n = watermark_payload_encode(&orig, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "encode positive");

    watermark_payload_t decoded;
    int rc = watermark_payload_decode(buf, (size_t)n, &decoded);
    TEST_ASSERT(rc == 0, "decode ok");
    TEST_ASSERT(decoded.viewer_id    == orig.viewer_id,    "viewer_id preserved");
    TEST_ASSERT(decoded.session_id   == orig.session_id,   "session_id preserved");
    TEST_ASSERT(decoded.timestamp_us == orig.timestamp_us, "timestamp preserved");
    TEST_ASSERT(decoded.payload_bits == 64, "payload_bits preserved");

    TEST_PASS("watermark_payload encode/decode round-trip");
    return 0;
}

static int test_payload_bad_magic(void) {
    printf("\n=== test_payload_bad_magic ===\n");

    uint8_t buf[WATERMARK_HDR_SIZE + WATERMARK_MAX_DATA_BYTES];
    memset(buf, 0, sizeof(buf));
    buf[0] = 0xFF;
    watermark_payload_t p;
    TEST_ASSERT(watermark_payload_decode(buf, sizeof(buf), &p) == -1,
                "bad magic returns -1");

    TEST_PASS("watermark_payload bad magic rejected");
    return 0;
}

static int test_payload_bits(void) {
    printf("\n=== test_payload_bits ===\n");

    watermark_payload_t p;
    memset(&p, 0, sizeof(p));
    p.viewer_id = 0xAAAAAAAAAAAAAAAAULL; /* alternating 1010... pattern */

    uint8_t bits[64];
    int n = watermark_payload_to_bits(&p, bits, 64);
    TEST_ASSERT(n == 64, "to_bits returns 64");

    /* MSB of 0xAAAA... is 1 */
    TEST_ASSERT(bits[0] == 1, "MSB is 1");
    TEST_ASSERT(bits[1] == 0, "next bit is 0");

    watermark_payload_t p2;
    int rc = watermark_payload_from_bits(bits, 64, &p2);
    TEST_ASSERT(rc == 0, "from_bits ok");
    TEST_ASSERT(p2.viewer_id == p.viewer_id, "viewer_id round-trips through bits");

    TEST_PASS("watermark_payload to/from bits");
    return 0;
}

static int test_payload_null_guards(void) {
    printf("\n=== test_payload_null_guards ===\n");

    uint8_t buf[64];
    watermark_payload_t p; memset(&p, 0, sizeof(p));
    TEST_ASSERT(watermark_payload_encode(NULL, buf, sizeof(buf)) == -1,
                "encode NULL payload");
    TEST_ASSERT(watermark_payload_encode(&p, NULL, 0) == -1,
                "encode NULL buf");
    TEST_ASSERT(watermark_payload_decode(NULL, 0, &p) == -1,
                "decode NULL buf");

    TEST_PASS("watermark_payload NULL guards");
    return 0;
}

/* ── watermark_lsb tests ─────────────────────────────────────────── */

#define FRAME_W 128
#define FRAME_H 64

static int test_lsb_embed_extract(void) {
    printf("\n=== test_lsb_embed_extract ===\n");

    uint8_t frame[FRAME_W * FRAME_H];
    memset(frame, 0x80, sizeof(frame)); /* grey frame */

    watermark_payload_t payload;
    memset(&payload, 0, sizeof(payload));
    payload.viewer_id    = 0xFEEDFACEDEADULL;
    payload.payload_bits = 64;

    int n = watermark_lsb_embed(frame, FRAME_W, FRAME_H, FRAME_W, &payload);
    TEST_ASSERT(n == 64, "embed 64 bits");

    watermark_payload_t extracted;
    n = watermark_lsb_extract(frame, FRAME_W, FRAME_H, FRAME_W,
                               payload.viewer_id, &extracted);
    TEST_ASSERT(n == 64, "extract 64 bits");
    TEST_ASSERT(extracted.viewer_id == payload.viewer_id,
                "viewer_id extracted correctly");

    TEST_PASS("watermark_lsb embed/extract round-trip");
    return 0;
}

static int test_lsb_invisibility(void) {
    printf("\n=== test_lsb_invisibility ===\n");

    /* LSB modification should change pixel values by at most 1 */
    uint8_t orig[FRAME_W * FRAME_H];
    uint8_t modified[FRAME_W * FRAME_H];
    for (int i = 0; i < FRAME_W * FRAME_H; i++)
        orig[i] = modified[i] = (uint8_t)(i & 0xFF);

    watermark_payload_t payload;
    memset(&payload, 0, sizeof(payload));
    payload.viewer_id = 0x123456789ABCULL;

    watermark_lsb_embed(modified, FRAME_W, FRAME_H, FRAME_W, &payload);

    int max_diff = 0;
    for (int i = 0; i < FRAME_W * FRAME_H; i++) {
        int d = abs((int)modified[i] - (int)orig[i]);
        if (d > max_diff) max_diff = d;
    }
    TEST_ASSERT(max_diff <= 1, "LSB embed: max pixel change <= 1");

    TEST_PASS("watermark_lsb invisibility (max change 1)");
    return 0;
}

static int test_lsb_null_guards(void) {
    printf("\n=== test_lsb_null_guards ===\n");

    watermark_payload_t p; memset(&p, 0, sizeof(p));
    uint8_t f[64] = {0};
    TEST_ASSERT(watermark_lsb_embed(NULL, 8, 8, 8, &p) == -1,  "embed NULL luma");
    TEST_ASSERT(watermark_lsb_embed(f, 8, 8, 8, NULL) == -1,   "embed NULL payload");
    TEST_ASSERT(watermark_lsb_embed(f, 1, 1, 1, &p) == -1,     "embed too small");
    TEST_ASSERT(watermark_lsb_extract(NULL, 8, 8, 8, 0, &p) == -1, "extract NULL");

    TEST_PASS("watermark_lsb NULL guards");
    return 0;
}

/* ── watermark_dct tests ─────────────────────────────────────────── */

/* DCT mode needs width >= 64*8 = 512, height >= 8 */
#define DCT_W 512
#define DCT_H 16

static int test_dct_embed_extract(void) {
    printf("\n=== test_dct_embed_extract ===\n");

    uint8_t *frame = calloc((size_t)(DCT_W * DCT_H), 1);
    TEST_ASSERT(frame != NULL, "frame alloc");

    /* Fill with mid-grey to give DCT coefficients meaningful values */
    memset(frame, 128, (size_t)(DCT_W * DCT_H));
    /* Add some variation so DCT isn't trivially uniform */
    for (int i = 0; i < DCT_W * DCT_H; i++)
        frame[i] = (uint8_t)(64 + (i % 64) * 2);

    watermark_payload_t payload;
    memset(&payload, 0, sizeof(payload));
    payload.viewer_id    = 0xCAFEBABE12345678ULL;
    payload.payload_bits = 64;

    int n = watermark_dct_embed(frame, DCT_W, DCT_H, DCT_W, &payload,
                                 WATERMARK_DCT_DELTA_DEFAULT);
    TEST_ASSERT(n == 64, "dct embed 64 bits");

    watermark_payload_t extracted;
    n = watermark_dct_extract(frame, DCT_W, DCT_H, DCT_W,
                               WATERMARK_DCT_DELTA_DEFAULT, &extracted);
    TEST_ASSERT(n == 64, "dct extract 64 bits");
    TEST_ASSERT(extracted.viewer_id == payload.viewer_id,
                "dct viewer_id round-trip");

    free(frame);
    TEST_PASS("watermark_dct embed/extract round-trip");
    return 0;
}

static int test_dct_null_guards(void) {
    printf("\n=== test_dct_null_guards ===\n");

    watermark_payload_t p; memset(&p, 0, sizeof(p));
    uint8_t f[8] = {0};
    TEST_ASSERT(watermark_dct_embed(NULL, 512, 8, 512, &p, 4) == -1, "embed NULL");
    TEST_ASSERT(watermark_dct_embed(f, 64, 8, 64, &p, 4) == -1, "embed too small");
    TEST_ASSERT(watermark_dct_extract(NULL, 512, 8, 512, 4, &p) == -1, "extract NULL");

    TEST_PASS("watermark_dct NULL/size guards");
    return 0;
}

/* ── watermark_strength tests ────────────────────────────────────── */

static int test_strength_high_quality(void) {
    printf("\n=== test_strength_high_quality ===\n");

    watermark_strength_t s;
    int rc = watermark_strength_select(85, true, &s);
    TEST_ASSERT(rc == 0, "select returns 0");
    TEST_ASSERT(s.mode == WATERMARK_MODE_LSB, "high quality → LSB mode");
    TEST_ASSERT(s.apply, "keyframe → apply=true");

    TEST_PASS("watermark_strength high quality → LSB");
    return 0;
}

static int test_strength_low_quality(void) {
    printf("\n=== test_strength_low_quality ===\n");

    watermark_strength_t s;
    watermark_strength_select(20, true, &s);
    TEST_ASSERT(s.mode == WATERMARK_MODE_DCT, "low quality → DCT mode");
    TEST_ASSERT(s.dct_delta > WATERMARK_DCT_DELTA_DEFAULT,
                "low quality → larger delta");

    TEST_PASS("watermark_strength low quality → DCT large delta");
    return 0;
}

static int test_strength_non_keyframe_dct(void) {
    printf("\n=== test_strength_non_keyframe_dct ===\n");

    watermark_strength_t s;
    watermark_strength_select(50, false, &s); /* mid quality, non-keyframe */
    TEST_ASSERT(s.mode == WATERMARK_MODE_DCT, "mid quality → DCT");
    TEST_ASSERT(!s.apply, "non-keyframe DCT → apply=false");

    TEST_PASS("watermark_strength: DCT skipped on non-keyframe");
    return 0;
}

static int test_strength_mode_name(void) {
    printf("\n=== test_strength_mode_name ===\n");

    TEST_ASSERT(strcmp(watermark_strength_mode_name(WATERMARK_MODE_LSB), "lsb") == 0,
                "LSB mode name");
    TEST_ASSERT(strcmp(watermark_strength_mode_name(WATERMARK_MODE_DCT), "dct") == 0,
                "DCT mode name");
    TEST_ASSERT(strcmp(watermark_strength_mode_name((watermark_mode_t)99), "unknown") == 0,
                "unknown mode name");

    TEST_PASS("watermark_strength mode names");
    return 0;
}

static int test_strength_null_guard(void) {
    printf("\n=== test_strength_null_guard ===\n");

    TEST_ASSERT(watermark_strength_select(80, true, NULL) == -1, "NULL out");

    TEST_PASS("watermark_strength NULL guard");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_payload_roundtrip();
    failures += test_payload_bad_magic();
    failures += test_payload_bits();
    failures += test_payload_null_guards();

    failures += test_lsb_embed_extract();
    failures += test_lsb_invisibility();
    failures += test_lsb_null_guards();

    failures += test_dct_embed_extract();
    failures += test_dct_null_guards();

    failures += test_strength_high_quality();
    failures += test_strength_low_quality();
    failures += test_strength_non_keyframe_dct();
    failures += test_strength_mode_name();
    failures += test_strength_null_guard();

    printf("\n");
    if (failures == 0)
        printf("ALL WATERMARK TESTS PASSED\n");
    else
        printf("%d WATERMARK TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
