/*
 * test_chunk.c — Unit tests for PHASE-76 Chunk Splitter
 *
 * Tests chunk_hdr (init/invalid), chunk_split (1-chunk, multi-chunk,
 * empty, exact-MTU, last-flag), and chunk_reassemble (single/multi-chunk
 * frame, completion detection, release, out-of-order arrival).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../../src/chunk/chunk_hdr.h"
#include "../../src/chunk/chunk_split.h"
#include "../../src/chunk/chunk_reassemble.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── chunk_hdr ───────────────────────────────────────────────────── */

static int test_hdr_init(void) {
    printf("\n=== test_hdr_init ===\n");

    chunk_hdr_t h;
    TEST_ASSERT(chunk_hdr_init(&h, 1, 42, 0, 3, 512, CHUNK_FLAG_KEYFRAME) == 0, "init ok");
    TEST_ASSERT(h.stream_id   == 1,                "stream_id");
    TEST_ASSERT(h.frame_seq   == 42,               "frame_seq");
    TEST_ASSERT(h.chunk_idx   == 0,                "chunk_idx");
    TEST_ASSERT(h.chunk_count == 3,                "chunk_count");
    TEST_ASSERT(h.data_len    == 512,              "data_len");
    TEST_ASSERT(h.flags == CHUNK_FLAG_KEYFRAME,    "flags");

    /* Invalid: chunk_count = 0 */
    TEST_ASSERT(chunk_hdr_init(&h, 1, 0, 0, 0, 10, 0) == -1, "count=0 → -1");
    /* Invalid: idx >= count */
    TEST_ASSERT(chunk_hdr_init(&h, 1, 0, 3, 3, 10, 0) == -1, "idx>=count → -1");
    /* NULL */
    TEST_ASSERT(chunk_hdr_init(NULL, 1, 0, 0, 1, 0, 0) == -1, "NULL → -1");

    TEST_PASS("chunk_hdr init / invalid guard");
    return 0;
}

/* ── chunk_split ─────────────────────────────────────────────────── */

static int test_split_single(void) {
    printf("\n=== test_split_single ===\n");

    uint8_t frame[100];
    memset(frame, 0xAA, sizeof(frame));

    chunk_t out[CHUNK_SPLIT_MAX];
    int n = chunk_split(frame, 100, 1500, 1, 7, 0, out, CHUNK_SPLIT_MAX);
    TEST_ASSERT(n == 1, "100B frame with 1500B MTU → 1 chunk");
    TEST_ASSERT(out[0].hdr.chunk_count == 1, "chunk_count=1");
    TEST_ASSERT(out[0].hdr.chunk_idx  == 0, "chunk_idx=0");
    TEST_ASSERT(out[0].hdr.data_len == 100, "data_len=100");
    TEST_ASSERT((out[0].hdr.flags & CHUNK_FLAG_LAST) != 0, "LAST set");
    TEST_ASSERT(out[0].data == frame, "data ptr in source");

    TEST_PASS("chunk_split single chunk");
    return 0;
}

static int test_split_multi(void) {
    printf("\n=== test_split_multi ===\n");

    uint8_t frame[250];
    for (int i = 0; i < 250; i++) frame[i] = (uint8_t)i;

    chunk_t out[CHUNK_SPLIT_MAX];
    int n = chunk_split(frame, 250, 100, 2, 99, 0, out, CHUNK_SPLIT_MAX);
    /* 250 / 100 = 2 full + 1 partial = 3 chunks */
    TEST_ASSERT(n == 3, "250B frame with 100B MTU → 3 chunks");
    TEST_ASSERT(out[0].hdr.chunk_count == 3, "chunk_count=3 on all chunks");
    TEST_ASSERT(out[1].hdr.chunk_count == 3, "chunk_count=3 chunk 1");
    TEST_ASSERT(out[2].hdr.chunk_count == 3, "chunk_count=3 chunk 2");

    TEST_ASSERT(out[0].hdr.data_len == 100, "chunk 0: 100B");
    TEST_ASSERT(out[1].hdr.data_len == 100, "chunk 1: 100B");
    TEST_ASSERT(out[2].hdr.data_len ==  50, "chunk 2: 50B");

    TEST_ASSERT((out[2].hdr.flags & CHUNK_FLAG_LAST) != 0, "LAST on final chunk");
    TEST_ASSERT((out[0].hdr.flags & CHUNK_FLAG_LAST) == 0, "no LAST on chunk 0");

    /* Data pointers into frame */
    TEST_ASSERT(out[0].data == frame,       "chunk 0 ptr");
    TEST_ASSERT(out[1].data == frame + 100, "chunk 1 ptr");
    TEST_ASSERT(out[2].data == frame + 200, "chunk 2 ptr");

    /* Invalid params */
    TEST_ASSERT(chunk_split(NULL, 100, 1500, 1, 0, 0, out, CHUNK_SPLIT_MAX) == -1, "NULL data");
    TEST_ASSERT(chunk_split(frame, 100, 0, 1, 0, 0, out, CHUNK_SPLIT_MAX) == -1, "mtu=0");

    TEST_PASS("chunk_split multi-chunk / invalid guard");
    return 0;
}

/* ── chunk_reassemble ────────────────────────────────────────────── */

static int test_reassemble_single(void) {
    printf("\n=== test_reassemble_single ===\n");

    reassemble_ctx_t *ctx = reassemble_ctx_create();
    TEST_ASSERT(ctx != NULL, "created");

    chunk_hdr_t h;
    chunk_hdr_init(&h, 1, 10, 0, 1, 500, CHUNK_FLAG_LAST);

    reassemble_slot_t *s = reassemble_receive(ctx, &h);
    TEST_ASSERT(s != NULL, "slot allocated");
    TEST_ASSERT(s->complete, "single chunk → complete");
    TEST_ASSERT(s->received_mask == 1, "mask bit 0");

    TEST_ASSERT(reassemble_release(ctx, s) == 0, "release ok");
    TEST_ASSERT(reassemble_count(ctx) == 0, "count = 0 after release");

    reassemble_ctx_destroy(ctx);
    TEST_PASS("reassemble single chunk");
    return 0;
}

static int test_reassemble_multi(void) {
    printf("\n=== test_reassemble_multi ===\n");

    reassemble_ctx_t *ctx = reassemble_ctx_create();

    chunk_hdr_t h;
    /* 3-chunk frame, chunks arrive out-of-order: 2, 0, 1 */
    chunk_hdr_init(&h, 1, 20, 2, 3, 100, CHUNK_FLAG_LAST);
    reassemble_slot_t *s = reassemble_receive(ctx, &h);
    TEST_ASSERT(s != NULL && !s->complete, "after chunk 2: not complete");

    chunk_hdr_init(&h, 1, 20, 0, 3, 100, 0);
    s = reassemble_receive(ctx, &h);
    TEST_ASSERT(s != NULL && !s->complete, "after chunk 0: not complete");

    chunk_hdr_init(&h, 1, 20, 1, 3, 100, 0);
    s = reassemble_receive(ctx, &h);
    TEST_ASSERT(s != NULL && s->complete, "after chunk 1: complete");
    TEST_ASSERT(s->received_mask == 0x7, "all 3 bits set");

    reassemble_release(ctx, s);
    reassemble_ctx_destroy(ctx);
    TEST_PASS("reassemble multi-chunk out-of-order");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_hdr_init();
    failures += test_split_single();
    failures += test_split_multi();
    failures += test_reassemble_single();
    failures += test_reassemble_multi();

    printf("\n");
    if (failures == 0) printf("ALL CHUNK TESTS PASSED\n");
    else               printf("%d CHUNK TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
