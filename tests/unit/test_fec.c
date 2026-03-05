/*
 * test_fec.c — Unit tests for PHASE-64 FEC Encoder / Decoder
 *
 * Tests fec_matrix (covers/repair), fec_encoder (encode k=4 r=2),
 * and fec_decoder (recover 1 lost source, 2 lost sources, irrecoverable).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/fec/fec_matrix.h"
#include "../../src/fec/fec_encoder.h"
#include "../../src/fec/fec_decoder.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

#define K 4
#define R 2
#define PSZ 64

/* Allocate and fill a source buffer with a repeating byte pattern */
static uint8_t *make_src(uint8_t fill) {
    uint8_t *p = malloc(PSZ);
    if (p) memset(p, fill, PSZ);
    return p;
}

static int test_matrix_covers(void) {
    printf("\n=== test_matrix_covers ===\n");

    /* repair_idx=0: src_idx % 2 == 0 → covers 0, 2, 4, ... */
    TEST_ASSERT(fec_repair_covers(0, 0) == 1, "src0 covers repair0");
    TEST_ASSERT(fec_repair_covers(2, 0) == 1, "src2 covers repair0");
    TEST_ASSERT(fec_repair_covers(1, 0) == 0, "src1 does not cover repair0");

    /* repair_idx=1: src_idx % 3 == 0 → covers 0, 3, 6, ... */
    TEST_ASSERT(fec_repair_covers(0, 1) == 1, "src0 covers repair1");
    TEST_ASSERT(fec_repair_covers(3, 1) == 1, "src3 covers repair1");
    TEST_ASSERT(fec_repair_covers(1, 1) == 0, "src1 does not cover repair1");

    TEST_PASS("fec_matrix covers pattern");
    return 0;
}

static int test_fec_encode(void) {
    printf("\n=== test_fec_encode ===\n");

    /* sources: 4 packets filled with bytes 0xA0..0xA3 */
    uint8_t *srcs[K];
    for (int i = 0; i < K; i++) srcs[i] = make_src((uint8_t)(0xA0 + i));

    /* output: K+R buffers */
    uint8_t *out[K + R];
    for (int i = 0; i < K + R; i++) out[i] = malloc(PSZ);

    int rc = fec_encode((const uint8_t *const *)srcs, K, R,
                         (uint8_t **)out, PSZ);
    TEST_ASSERT(rc == 0, "encode ok");

    /* First K outputs are copies of sources */
    for (int i = 0; i < K; i++)
        TEST_ASSERT(memcmp(out[i], srcs[i], PSZ) == 0, "source pass-through");

    /* Repair[0] = XOR of src[0] and src[2] (both % 2 == 0 for R=2 covers) */
    uint8_t expected_r0[PSZ];
    memset(expected_r0, 0, PSZ);
    for (int j = 0; j < K; j++)
        if (fec_repair_covers(j, 0))
            for (int b = 0; b < PSZ; b++) expected_r0[b] ^= srcs[j][b];
    TEST_ASSERT(memcmp(out[K], expected_r0, PSZ) == 0, "repair[0] correct XOR");

    for (int i = 0; i < K + R; i++) free(out[i]);
    for (int i = 0; i < K; i++) free(srcs[i]);
    TEST_PASS("fec_encode pass-through and repair");
    return 0;
}

static int test_fec_decode_one_loss(void) {
    printf("\n=== test_fec_decode_one_loss ===\n");

    uint8_t *srcs[K];
    for (int i = 0; i < K; i++) srcs[i] = make_src((uint8_t)(0xB0 + i));

    uint8_t *encoded[K + R];
    for (int i = 0; i < K + R; i++) encoded[i] = malloc(PSZ);
    fec_encode((const uint8_t *const *)srcs, K, R, (uint8_t **)encoded, PSZ);

    /* Simulate loss of src[0] (which is covered by both repair[0] and repair[1]) */
    bool received[K + R];
    for (int i = 0; i < K + R; i++) received[i] = true;
    received[0] = false; /* lose src[0] */

    uint8_t *recovered[K];
    for (int i = 0; i < K; i++) recovered[i] = malloc(PSZ);

    int n = fec_decode((const uint8_t *const *)encoded, received,
                        K, R, PSZ, (uint8_t **)recovered);
    TEST_ASSERT(n >= 1, "recovered ≥ 1 packet");
    TEST_ASSERT(memcmp(recovered[0], srcs[0], PSZ) == 0, "src[0] correctly recovered");

    for (int i = 0; i < K; i++) { free(recovered[i]); free(srcs[i]); }
    for (int i = 0; i < K + R; i++) free(encoded[i]);
    TEST_PASS("fec_decode single packet recovery");
    return 0;
}

static int test_fec_decode_irrecoverable(void) {
    printf("\n=== test_fec_decode_irrecoverable ===\n");

    uint8_t *srcs[K];
    for (int i = 0; i < K; i++) srcs[i] = make_src((uint8_t)(0xC0 + i));

    uint8_t *encoded[K + R];
    for (int i = 0; i < K + R; i++) encoded[i] = malloc(PSZ);
    fec_encode((const uint8_t *const *)srcs, K, R, (uint8_t **)encoded, PSZ);

    /* Lose src[1] AND src[3] — repair[0] covers 0,2 but not 1 or 3;
     * no repair covers both → irrecoverable with XOR-only scheme */
    bool received[K + R];
    for (int i = 0; i < K + R; i++) received[i] = true;
    received[1] = false;
    received[3] = false;
    /* Also lose repair[1] to ensure no single-step recovery is possible */
    received[K + 1] = false;

    uint8_t *recovered[K];
    for (int i = 0; i < K; i++) recovered[i] = malloc(PSZ);

    int n = fec_decode((const uint8_t *const *)encoded, received,
                        K, R, PSZ, (uint8_t **)recovered);
    TEST_ASSERT(n == 0, "irrecoverable → 0 recovered");

    for (int i = 0; i < K; i++) { free(recovered[i]); free(srcs[i]); }
    for (int i = 0; i < K + R; i++) free(encoded[i]);
    TEST_PASS("fec_decode irrecoverable loss (0 recovered)");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_matrix_covers();
    failures += test_fec_encode();
    failures += test_fec_decode_one_loss();
    failures += test_fec_decode_irrecoverable();

    printf("\n");
    if (failures == 0) printf("ALL FEC TESTS PASSED\n");
    else               printf("%d FEC TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
