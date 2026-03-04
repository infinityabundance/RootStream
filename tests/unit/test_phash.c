/*
 * test_phash.c — Unit tests for PHASE-46 Perceptual Frame Hashing
 *
 * Tests phash (compute/hamming/similar), phash_index (insert/nearest/range),
 * and phash_dedup (push/reset/count).  Generates synthetic luma frames
 * in memory; no video hardware required.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/phash/phash.h"
#include "../../src/phash/phash_index.h"
#include "../../src/phash/phash_dedup.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── Synthetic frame generators ─────────────────────────────────── */

#define FRAME_W  64
#define FRAME_H  64

/* Flat grey frame at intensity @v */
static void make_grey(uint8_t *out, int w, int h, uint8_t v) {
    memset(out, v, (size_t)(w * h));
}

/* Horizontal gradient: left=0, right=255 */
static void make_hgrad(uint8_t *out, int w, int h) {
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            out[y * w + x] = (uint8_t)((x * 255) / (w - 1));
}

/* Checkerboard of 8×8 blocks alternating 0/255 */
static void make_checker(uint8_t *out, int w, int h) {
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            out[y * w + x] = (uint8_t)(((x / 8 + y / 8) & 1) ? 255 : 0);
}

/* ── phash tests ─────────────────────────────────────────────────── */

static int test_phash_deterministic(void) {
    printf("\n=== test_phash_deterministic ===\n");

    uint8_t frame[FRAME_W * FRAME_H];
    make_hgrad(frame, FRAME_W, FRAME_H);

    uint64_t h1, h2;
    int rc = phash_compute(frame, FRAME_W, FRAME_H, FRAME_W, &h1);
    TEST_ASSERT(rc == 0, "phash_compute returns 0");
    rc = phash_compute(frame, FRAME_W, FRAME_H, FRAME_W, &h2);
    TEST_ASSERT(h1 == h2, "same frame produces same hash");

    TEST_PASS("phash deterministic");
    return 0;
}

static int test_phash_identical_frames(void) {
    printf("\n=== test_phash_identical_frames ===\n");

    uint8_t f1[FRAME_W * FRAME_H], f2[FRAME_W * FRAME_H];
    make_grey(f1, FRAME_W, FRAME_H, 128);
    make_grey(f2, FRAME_W, FRAME_H, 128);

    uint64_t h1, h2;
    phash_compute(f1, FRAME_W, FRAME_H, FRAME_W, &h1);
    phash_compute(f2, FRAME_W, FRAME_H, FRAME_W, &h2);

    int dist = phash_hamming(h1, h2);
    TEST_ASSERT(dist == 0, "identical frames: Hamming dist = 0");
    TEST_ASSERT(phash_similar(h1, h2, 5), "similar predicate true for identical");

    TEST_PASS("phash identical frames");
    return 0;
}

static int test_phash_different_frames(void) {
    printf("\n=== test_phash_different_frames ===\n");

    uint8_t f1[FRAME_W * FRAME_H], f2[FRAME_W * FRAME_H];
    make_grey(f1, FRAME_W, FRAME_H, 0);   /* all black */
    make_checker(f2, FRAME_W, FRAME_H);   /* checkerboard */

    uint64_t h1, h2;
    phash_compute(f1, FRAME_W, FRAME_H, FRAME_W, &h1);
    phash_compute(f2, FRAME_W, FRAME_H, FRAME_W, &h2);

    int dist = phash_hamming(h1, h2);
    /* Different scenes should differ significantly */
    TEST_ASSERT(dist > 5, "different frames: Hamming dist > 5");
    TEST_ASSERT(!phash_similar(h1, h2, 5), "similar predicate false for different");

    TEST_PASS("phash different frames");
    return 0;
}

static int test_phash_hamming(void) {
    printf("\n=== test_phash_hamming ===\n");

    TEST_ASSERT(phash_hamming(0x0, 0x0) == 0,  "0 ^ 0 = 0 bits");
    TEST_ASSERT(phash_hamming(0x0, 0x1) == 1,  "0 ^ 1 = 1 bit");
    TEST_ASSERT(phash_hamming(0xFFFFFFFFFFFFFFFFULL, 0x0) == 64,
                "all bits differ = 64");
    TEST_ASSERT(phash_hamming(0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL) == 64,
                "alternating pattern = 64");

    TEST_PASS("phash Hamming distance");
    return 0;
}

static int test_phash_null_guards(void) {
    printf("\n=== test_phash_null_guards ===\n");

    uint64_t h;
    uint8_t f[64] = {0};
    TEST_ASSERT(phash_compute(NULL, 8, 8, 8, &h) == -1, "NULL luma");
    TEST_ASSERT(phash_compute(f, 8, 8, 8, NULL) == -1, "NULL out");
    TEST_ASSERT(phash_compute(f, 0, 8, 8, &h) == -1, "zero width");
    TEST_ASSERT(phash_compute(f, 8, 8, 4, &h) == -1, "stride < width");

    TEST_PASS("phash null/invalid guards");
    return 0;
}

/* ── phash_index tests ───────────────────────────────────────────── */

static int test_index_insert_nearest(void) {
    printf("\n=== test_index_insert_nearest ===\n");

    phash_index_t *idx = phash_index_create();
    TEST_ASSERT(idx != NULL, "index created");
    TEST_ASSERT(phash_index_count(idx) == 0, "initial count 0");

    /* Insert three hashes */
    phash_index_insert(idx, 0x0000000000000000ULL, 100);
    phash_index_insert(idx, 0xFFFFFFFFFFFFFFFFULL, 200);
    phash_index_insert(idx, 0x0000000000000001ULL, 300);
    TEST_ASSERT(phash_index_count(idx) == 3, "count 3");

    /* Nearest to 0x0 should be 0x0 (id=100, dist=0) */
    uint64_t match_id; int dist;
    int rc = phash_index_nearest(idx, 0x0, &match_id, &dist);
    TEST_ASSERT(rc == 0, "nearest found");
    TEST_ASSERT(match_id == 100, "nearest id correct");
    TEST_ASSERT(dist == 0, "nearest dist 0");

    /* Nearest to 0x3 should be 0x1 (dist=1) not 0xFF...FF (dist=62) */
    rc = phash_index_nearest(idx, 0x3ULL, &match_id, &dist);
    TEST_ASSERT(rc == 0, "nearest of 0x3 found");
    TEST_ASSERT(dist == 1, "nearest dist 1");

    phash_index_destroy(idx);
    TEST_PASS("phash_index insert/nearest");
    return 0;
}

static int test_index_range_query(void) {
    printf("\n=== test_index_range_query ===\n");

    phash_index_t *idx = phash_index_create();
    phash_index_insert(idx, 0x0000000000000000ULL, 1);
    phash_index_insert(idx, 0x0000000000000001ULL, 2);  /* dist 1 from 0 */
    phash_index_insert(idx, 0x0000000000000003ULL, 3);  /* dist 2 from 0 */
    phash_index_insert(idx, 0xFFFFFFFFFFFFFFFFULL, 4);  /* very different */

    phash_entry_t out[8];
    size_t n = phash_index_range_query(idx, 0x0, 2, out, 8);
    TEST_ASSERT(n == 3, "3 entries within dist 2 of 0x0");

    n = phash_index_range_query(idx, 0x0, 0, out, 8);
    TEST_ASSERT(n == 1, "exactly 1 entry within dist 0");

    phash_index_destroy(idx);
    TEST_PASS("phash_index range query");
    return 0;
}

static int test_index_remove(void) {
    printf("\n=== test_index_remove ===\n");

    phash_index_t *idx = phash_index_create();
    phash_index_insert(idx, 0x1ULL, 42);
    TEST_ASSERT(phash_index_count(idx) == 1, "1 entry");

    int rc = phash_index_remove(idx, 42);
    TEST_ASSERT(rc == 0, "remove returns 0");
    TEST_ASSERT(phash_index_count(idx) == 0, "0 after remove");

    rc = phash_index_remove(idx, 99);
    TEST_ASSERT(rc == -1, "remove nonexistent returns -1");

    phash_index_destroy(idx);
    TEST_PASS("phash_index remove");
    return 0;
}

/* ── phash_dedup tests ───────────────────────────────────────────── */

static int test_dedup_unique(void) {
    printf("\n=== test_dedup_unique ===\n");

    phash_dedup_t *d = phash_dedup_create(5);
    TEST_ASSERT(d != NULL, "dedup created");

    /* Very different hashes — should all be unique */
    bool dup = phash_dedup_push(d, 0x0000000000000000ULL, 1, NULL);
    TEST_ASSERT(!dup, "first frame unique");
    TEST_ASSERT(phash_dedup_indexed_count(d) == 1, "1 indexed");

    dup = phash_dedup_push(d, 0xFFFFFFFFFFFFFFFFULL, 2, NULL);
    TEST_ASSERT(!dup, "very different frame unique");
    TEST_ASSERT(phash_dedup_indexed_count(d) == 2, "2 indexed");

    phash_dedup_destroy(d);
    TEST_PASS("phash_dedup unique frames");
    return 0;
}

static int test_dedup_duplicate(void) {
    printf("\n=== test_dedup_duplicate ===\n");

    phash_dedup_t *d = phash_dedup_create(5);

    /* Push original */
    phash_dedup_push(d, 0x0000000000000000ULL, 1, NULL);

    /* Push a near-duplicate (Hamming dist = 2) */
    uint64_t match = 0;
    bool dup = phash_dedup_push(d, 0x0000000000000003ULL, 2, &match);
    TEST_ASSERT(dup, "near-duplicate detected");
    TEST_ASSERT(match == 1, "match points to original frame");
    TEST_ASSERT(phash_dedup_indexed_count(d) == 1, "duplicate not indexed");

    phash_dedup_destroy(d);
    TEST_PASS("phash_dedup duplicate detection");
    return 0;
}

static int test_dedup_reset(void) {
    printf("\n=== test_dedup_reset ===\n");

    phash_dedup_t *d = phash_dedup_create(5);
    phash_dedup_push(d, 0xABCDULL, 1, NULL);
    phash_dedup_push(d, 0x1234ULL, 2, NULL);
    TEST_ASSERT(phash_dedup_indexed_count(d) == 2, "2 before reset");

    phash_dedup_reset(d);
    TEST_ASSERT(phash_dedup_indexed_count(d) == 0, "0 after reset");

    /* Now the same hash as before is unique again */
    bool dup = phash_dedup_push(d, 0xABCDULL, 10, NULL);
    TEST_ASSERT(!dup, "after reset, previously-seen hash is unique");

    phash_dedup_destroy(d);
    TEST_PASS("phash_dedup reset");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_phash_deterministic();
    failures += test_phash_identical_frames();
    failures += test_phash_different_frames();
    failures += test_phash_hamming();
    failures += test_phash_null_guards();

    failures += test_index_insert_nearest();
    failures += test_index_range_query();
    failures += test_index_remove();

    failures += test_dedup_unique();
    failures += test_dedup_duplicate();
    failures += test_dedup_reset();

    printf("\n");
    if (failures == 0)
        printf("ALL PHASH TESTS PASSED\n");
    else
        printf("%d PHASH TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
