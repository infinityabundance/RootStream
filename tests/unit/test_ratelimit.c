/*
 * test_ratelimit.c — Unit tests for PHASE-52 Token Bucket Rate Limiter
 *
 * Tests token_bucket (create/consume/refill/available/reset/set_rate),
 * rate_limiter (add/remove/consume/has viewer), and ratelimit_stats
 * (record/snapshot/reset/throttle_rate).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/ratelimit/token_bucket.h"
#include "../../src/ratelimit/rate_limiter.h"
#include "../../src/ratelimit/ratelimit_stats.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── token_bucket tests ──────────────────────────────────────────── */

static int test_bucket_create(void) {
    printf("\n=== test_bucket_create ===\n");

    token_bucket_t *tb = token_bucket_create(1e6, 10000.0, 0);
    TEST_ASSERT(tb != NULL, "bucket created");
    TEST_ASSERT(token_bucket_available(tb, 0) == 10000.0, "starts full");

    token_bucket_destroy(tb);
    token_bucket_destroy(NULL); /* must not crash */

    TEST_ASSERT(token_bucket_create(0.0, 1.0, 0) == NULL, "rate=0 → NULL");
    TEST_ASSERT(token_bucket_create(1e6, 0.0, 0) == NULL, "burst=0 → NULL");

    TEST_PASS("token_bucket create/destroy");
    return 0;
}

static int test_bucket_consume(void) {
    printf("\n=== test_bucket_consume ===\n");

    /* 1 Mbps rate, 5000-byte burst, start at t=0 */
    token_bucket_t *tb = token_bucket_create(1e6, 5000.0, 0);

    /* Consume 3000 bytes: should succeed (bucket = 5000) */
    bool ok = token_bucket_consume(tb, 3000.0, 0);
    TEST_ASSERT(ok, "consume 3000 from 5000 ok");
    TEST_ASSERT(fabs(token_bucket_available(tb, 0) - 2000.0) < 1.0,
                "2000 tokens remaining");

    /* Consume 3000 more: should fail (only 2000 left) */
    ok = token_bucket_consume(tb, 3000.0, 0);
    TEST_ASSERT(!ok, "consume 3000 from 2000 fails");

    /* After 2ms (2000µs) at 1Mbps = 2000 new tokens → now 4000 */
    ok = token_bucket_consume(tb, 3000.0, 2000);
    TEST_ASSERT(ok, "consume 3000 after 2ms refill ok");

    token_bucket_destroy(tb);
    TEST_PASS("token_bucket consume/refill");
    return 0;
}

static int test_bucket_reset(void) {
    printf("\n=== test_bucket_reset ===\n");

    token_bucket_t *tb = token_bucket_create(1e6, 1000.0, 0);
    token_bucket_consume(tb, 800.0, 0);
    TEST_ASSERT(token_bucket_available(tb, 0) < 300.0, "depleted");

    token_bucket_reset(tb, 0);
    TEST_ASSERT(fabs(token_bucket_available(tb, 0) - 1000.0) < 1.0,
                "full after reset");

    token_bucket_destroy(tb);
    TEST_PASS("token_bucket reset");
    return 0;
}

static int test_bucket_set_rate(void) {
    printf("\n=== test_bucket_set_rate ===\n");

    token_bucket_t *tb = token_bucket_create(1e6, 5000.0, 0);
    token_bucket_consume(tb, 4000.0, 0);

    /* Change rate to 2 Mbps; set_rate refills at old rate (1Mbps) for 1ms first:
     * 1000 remaining + 1000 = 2000 tokens, then rate is updated.
     * Calling available(tb, 1000) afterwards reflects 2000 tokens. */
    int rc = token_bucket_set_rate(tb, 2e6, 1000);
    TEST_ASSERT(rc == 0, "set_rate ok");
    double avail = token_bucket_available(tb, 1000);
    TEST_ASSERT(avail >= 1900.0 && avail <= 2100.0, "~2000 tokens after rate change");

    TEST_ASSERT(token_bucket_set_rate(NULL, 1e6, 0) == -1, "NULL → -1");
    TEST_ASSERT(token_bucket_set_rate(tb, 0.0, 0) == -1, "rate=0 → -1");

    token_bucket_destroy(tb);
    TEST_PASS("token_bucket set_rate");
    return 0;
}

/* ── rate_limiter tests ───────────────────────────────────────────── */

static int test_rl_create(void) {
    printf("\n=== test_rl_create ===\n");

    rate_limiter_t *rl = rate_limiter_create(1e6, 5000.0);
    TEST_ASSERT(rl != NULL, "rate_limiter created");
    TEST_ASSERT(rate_limiter_viewer_count(rl) == 0, "initial count 0");

    rate_limiter_destroy(rl);
    rate_limiter_destroy(NULL);
    TEST_ASSERT(rate_limiter_create(0.0, 1.0) == NULL, "rate=0 → NULL");

    TEST_PASS("rate_limiter create/destroy");
    return 0;
}

static int test_rl_add_remove(void) {
    printf("\n=== test_rl_add_remove ===\n");

    rate_limiter_t *rl = rate_limiter_create(1e6, 5000.0);

    int rc = rate_limiter_add_viewer(rl, 0xABC, 0);
    TEST_ASSERT(rc == 0, "add viewer ok");
    TEST_ASSERT(rate_limiter_viewer_count(rl) == 1, "count 1");
    TEST_ASSERT(rate_limiter_has_viewer(rl, 0xABC), "has viewer");

    /* Duplicate add → no-op */
    rc = rate_limiter_add_viewer(rl, 0xABC, 0);
    TEST_ASSERT(rc == 0, "duplicate add ok");
    TEST_ASSERT(rate_limiter_viewer_count(rl) == 1, "count still 1");

    rc = rate_limiter_remove_viewer(rl, 0xABC);
    TEST_ASSERT(rc == 0, "remove ok");
    TEST_ASSERT(!rate_limiter_has_viewer(rl, 0xABC), "viewer gone");

    rc = rate_limiter_remove_viewer(rl, 0xABC);
    TEST_ASSERT(rc == -1, "remove non-existent → -1");

    rate_limiter_destroy(rl);
    TEST_PASS("rate_limiter add/remove");
    return 0;
}

static int test_rl_consume(void) {
    printf("\n=== test_rl_consume ===\n");

    rate_limiter_t *rl = rate_limiter_create(1e6, 5000.0);
    rate_limiter_add_viewer(rl, 1, 0);
    rate_limiter_add_viewer(rl, 2, 0);

    /* Viewer 1: consume entire burst */
    TEST_ASSERT(rate_limiter_consume(rl, 1, 5000.0, 0), "v1: consume 5000 ok");
    TEST_ASSERT(!rate_limiter_consume(rl, 1, 1.0, 0), "v1: throttled after burst");

    /* Viewer 2 is unaffected — has its own bucket */
    TEST_ASSERT(rate_limiter_consume(rl, 2, 4000.0, 0), "v2: independent bucket");

    /* Unknown viewer */
    TEST_ASSERT(!rate_limiter_consume(rl, 99, 1.0, 0), "unknown viewer → false");

    rate_limiter_destroy(rl);
    TEST_PASS("rate_limiter per-viewer buckets");
    return 0;
}

/* ── ratelimit_stats tests ───────────────────────────────────────── */

static int test_rlstats_record(void) {
    printf("\n=== test_rlstats_record ===\n");

    ratelimit_stats_t *st = ratelimit_stats_create();
    TEST_ASSERT(st != NULL, "stats created");

    ratelimit_stats_record(st, 1, 1500.0);
    ratelimit_stats_record(st, 1, 1200.0);
    ratelimit_stats_record(st, 0, 0.0); /* throttled */

    ratelimit_stats_snapshot_t snap;
    int rc = ratelimit_stats_snapshot(st, &snap);
    TEST_ASSERT(rc == 0, "snapshot ok");
    TEST_ASSERT(snap.packets_allowed == 2, "2 allowed");
    TEST_ASSERT(snap.packets_throttled == 1, "1 throttled");
    TEST_ASSERT(fabs(snap.bytes_consumed - 2700.0) < 1.0, "2700 bytes consumed");
    TEST_ASSERT(fabs(snap.throttle_rate - (1.0/3.0)) < 0.01, "throttle rate ~0.33");

    ratelimit_stats_reset(st);
    ratelimit_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.packets_allowed == 0, "reset clears allowed");

    ratelimit_stats_destroy(st);
    TEST_PASS("ratelimit_stats record/snapshot/reset");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_bucket_create();
    failures += test_bucket_consume();
    failures += test_bucket_reset();
    failures += test_bucket_set_rate();

    failures += test_rl_create();
    failures += test_rl_add_remove();
    failures += test_rl_consume();

    failures += test_rlstats_record();

    printf("\n");
    if (failures == 0)
        printf("ALL RATELIMIT TESTS PASSED\n");
    else
        printf("%d RATELIMIT TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
