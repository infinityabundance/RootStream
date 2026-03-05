/*
 * test_ladder.c — Unit tests for PHASE-69 Bitrate Ladder Builder
 *
 * Tests ladder_rung (init/compare/qsort), ladder_builder (build/
 * ascending/count/fps-reduce), and ladder_selector (select best rung
 * with margin).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../../src/ladder/ladder_rung.h"
#include "../../src/ladder/ladder_builder.h"
#include "../../src/ladder/ladder_selector.h"

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

/* ── ladder_rung ─────────────────────────────────────────────────── */

static int test_rung_init(void) {
    printf("\n=== test_rung_init ===\n");

    ladder_rung_t r;
    TEST_ASSERT(lr_init(&r, 4000000, 1280, 720, 30.0f) == 0, "init ok");
    TEST_ASSERT(r.bitrate_bps == 4000000, "bitrate");
    TEST_ASSERT(r.width == 1280, "width");
    TEST_ASSERT(r.height == 720, "height");

    TEST_ASSERT(lr_init(NULL, 1, 1, 1, 1) == -1, "NULL → -1");
    TEST_ASSERT(lr_init(&r, 0, 1280, 720, 30.0f) == -1, "bps=0 → -1");
    TEST_ASSERT(lr_init(&r, 1, 0, 720, 30.0f) == -1, "w=0 → -1");
    TEST_ASSERT(lr_init(&r, 1, 1280, 0, 30.0f) == -1, "h=0 → -1");
    TEST_ASSERT(lr_init(&r, 1, 1280, 720, 0.0f) == -1, "fps=0 → -1");

    /* Compare / qsort */
    ladder_rung_t rungs[3];
    lr_init(&rungs[0], 3000000, 1280, 720, 30.0f);
    lr_init(&rungs[1], 1000000,  640, 360, 30.0f);
    lr_init(&rungs[2], 6000000, 1920,1080, 30.0f);
    qsort(rungs, 3, sizeof(ladder_rung_t), lr_compare);
    TEST_ASSERT(rungs[0].bitrate_bps == 1000000, "sorted[0] = 1M");
    TEST_ASSERT(rungs[1].bitrate_bps == 3000000, "sorted[1] = 3M");
    TEST_ASSERT(rungs[2].bitrate_bps == 6000000, "sorted[2] = 6M");

    TEST_PASS("ladder_rung init / compare / qsort");
    return 0;
}

/* ── ladder_builder ──────────────────────────────────────────────── */

static int test_builder(void) {
    printf("\n=== test_builder ===\n");

    ladder_params_t p = {
        .max_bps              = 6000000,
        .min_bps              = 500000,
        .step_ratio           = 0.5f,
        .max_height           = 1080,
        .max_fps              = 30.0f,
        .fps_reduce_threshold = 0,  /* no fps reduction */
    };

    ladder_rung_t rungs[LADDER_MAX_RUNGS];
    int n = 0;
    TEST_ASSERT(ladder_build(&p, rungs, &n) == 0, "build ok");
    TEST_ASSERT(n >= 2, "at least 2 rungs");
    TEST_ASSERT(n <= LADDER_MAX_RUNGS, "≤ LADDER_MAX_RUNGS");

    /* Rungs should be ascending by bitrate */
    for (int i = 1; i < n; i++)
        TEST_ASSERT(rungs[i].bitrate_bps >= rungs[i-1].bitrate_bps,
                    "ascending bitrate order");

    /* All bitrates within [min_bps, max_bps] */
    for (int i = 0; i < n; i++) {
        TEST_ASSERT(rungs[i].bitrate_bps >= p.min_bps, "≥ min_bps");
        TEST_ASSERT(rungs[i].bitrate_bps <= p.max_bps, "≤ max_bps");
    }

    /* Invalid params */
    TEST_ASSERT(ladder_build(NULL, rungs, &n) == -1, "NULL → -1");
    ladder_params_t bad = p; bad.step_ratio = 0.0f;
    TEST_ASSERT(ladder_build(&bad, rungs, &n) == -1, "step_ratio=0 → -1");
    bad = p; bad.max_bps = 0;
    TEST_ASSERT(ladder_build(&bad, rungs, &n) == -1, "max_bps=0 → -1");

    TEST_PASS("ladder_builder build / ascending / range check");
    return 0;
}

/* ── ladder_selector ─────────────────────────────────────────────── */

static int test_selector(void) {
    printf("\n=== test_selector ===\n");

    /* Simple 3-rung ladder: 1M, 3M, 6M */
    ladder_rung_t rungs[3];
    lr_init(&rungs[0], 1000000,  640, 360, 30.0f);
    lr_init(&rungs[1], 3000000, 1280, 720, 30.0f);
    lr_init(&rungs[2], 6000000, 1920,1080, 30.0f);

    /* 5Mbps available, 20% margin → budget = 4Mbps → rung[1] (3M) */
    int idx = ladder_select(rungs, 3, 5000000, 0.20f);
    TEST_ASSERT(idx == 1, "5Mbps 20% margin → rung[1] (3M)");

    /* 7Mbps available, 0% margin → budget = 7Mbps → rung[2] (6M) */
    idx = ladder_select(rungs, 3, 7000000, 0.0f);
    TEST_ASSERT(idx == 2, "7Mbps 0% → rung[2] (6M)");

    /* 800kbps → only rung[0] (1M) fits if margin=0 → rung 0 */
    idx = ladder_select(rungs, 3, 800000, 0.0f);
    TEST_ASSERT(idx == 0, "800kbps → rung[0] (1M)");

    /* NULL rungs → 0 */
    idx = ladder_select(NULL, 3, 5000000, 0.0f);
    TEST_ASSERT(idx == 0, "NULL → 0");

    TEST_PASS("ladder_selector select / margin / fallback");
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_rung_init();
    failures += test_builder();
    failures += test_selector();

    printf("\n");
    if (failures == 0) printf("ALL LADDER TESTS PASSED\n");
    else               printf("%d LADDER TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
