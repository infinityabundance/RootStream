/*
 * test_mixer.c — Unit tests for PHASE-59 Multi-Stream Mixer
 *
 * Tests mix_source (init/set_weight/set_muted/type_names),
 * mix_engine (add/remove/update/duplicate/full/mix/clip/mute/silence),
 * and mix_stats (record/underrun/snapshot/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/mixer/mix_source.h"
#include "../../src/mixer/mix_engine.h"
#include "../../src/mixer/mix_stats.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

/* ── mix_source tests ────────────────────────────────────────────── */

static int test_source_init(void) {
    printf("\n=== test_source_init ===\n");

    mix_source_t src;
    int rc = mix_source_init(&src, 1, MIX_SRC_MICROPHONE, 1.0f, "mic");
    TEST_ASSERT(rc == 0,           "init ok");
    TEST_ASSERT(src.id == 1,       "id");
    TEST_ASSERT(src.type == MIX_SRC_MICROPHONE, "type");
    TEST_ASSERT(fabsf(src.weight - 1.0f) < 1e-6f, "weight 1.0");
    TEST_ASSERT(!src.muted,        "not muted");
    TEST_ASSERT(strcmp(src.name, "mic") == 0, "name");

    /* Weight clamp */
    mix_source_init(&src, 2, MIX_SRC_SYNTH, 99.0f, "loud");
    TEST_ASSERT(fabsf(src.weight - MIX_WEIGHT_MAX) < 1e-6f, "weight clamped to MAX");

    mix_source_init(&src, 3, MIX_SRC_SYNTH, -1.0f, "neg");
    TEST_ASSERT(fabsf(src.weight) < 1e-6f, "weight clamped to 0");

    TEST_ASSERT(mix_source_init(NULL, 1, MIX_SRC_SYNTH, 1.0f, "x") == -1, "NULL → -1");

    TEST_PASS("mix_source init / weight-clamp");
    return 0;
}

static int test_source_mutate(void) {
    printf("\n=== test_source_mutate ===\n");

    mix_source_t src;
    mix_source_init(&src, 10, MIX_SRC_CAPTURE, 1.0f, "cap");

    mix_source_set_muted(&src, true);
    TEST_ASSERT(src.muted, "muted");
    mix_source_set_muted(&src, false);
    TEST_ASSERT(!src.muted, "unmuted");

    mix_source_set_weight(&src, 2.0f);
    TEST_ASSERT(fabsf(src.weight - 2.0f) < 1e-6f, "weight 2.0");

    TEST_ASSERT(mix_src_type_name(MIX_SRC_CAPTURE)    != NULL, "CAPTURE name");
    TEST_ASSERT(strcmp(mix_src_type_name(MIX_SRC_LOOPBACK), "LOOPBACK") == 0, "LOOPBACK name");
    TEST_ASSERT(strcmp(mix_src_type_name((mix_src_type_t)99), "UNKNOWN") == 0, "unknown");

    TEST_PASS("mix_source set_muted / set_weight / type names");
    return 0;
}

/* ── mix_engine tests ────────────────────────────────────────────── */

static mix_source_t make_src(uint32_t id, float weight, bool muted) {
    mix_source_t s;
    mix_source_init(&s, id, MIX_SRC_SYNTH, weight, "src");
    s.muted = muted;
    return s;
}

static int test_engine_add_remove(void) {
    printf("\n=== test_engine_add_remove ===\n");

    mix_engine_t *e = mix_engine_create();
    TEST_ASSERT(e != NULL, "engine created");
    TEST_ASSERT(mix_engine_source_count(e) == 0, "initially 0 sources");

    mix_source_t s1 = make_src(1, 1.0f, false);
    mix_source_t s2 = make_src(2, 1.0f, false);
    TEST_ASSERT(mix_engine_add_source(e, &s1) == 0, "add s1");
    TEST_ASSERT(mix_engine_add_source(e, &s2) == 0, "add s2");
    TEST_ASSERT(mix_engine_source_count(e) == 2, "2 sources");

    /* Duplicate ID */
    TEST_ASSERT(mix_engine_add_source(e, &s1) == -1, "dup add → -1");

    TEST_ASSERT(mix_engine_remove_source(e, 1) == 0, "remove s1");
    TEST_ASSERT(mix_engine_source_count(e) == 1, "1 source after remove");
    TEST_ASSERT(mix_engine_remove_source(e, 99) == -1, "remove unknown → -1");

    mix_engine_destroy(e);
    TEST_PASS("mix_engine add / remove / duplicate guard");
    return 0;
}

static int test_engine_mix_basic(void) {
    printf("\n=== test_engine_mix_basic ===\n");

    mix_engine_t *e = mix_engine_create();

    mix_source_t s1 = make_src(1, 1.0f, false);
    mix_source_t s2 = make_src(2, 1.0f, false);
    mix_engine_add_source(e, &s1);
    mix_engine_add_source(e, &s2);

    /* Two sources with samples of 100 and 200 → mixed = 300 */
    int16_t in1[4] = {100, 100, 100, 100};
    int16_t in2[4] = {200, 200, 200, 200};
    const int16_t *inputs[2] = {in1, in2};
    uint32_t ids[2] = {1, 2};
    int16_t  out[4] = {0};

    int rc = mix_engine_mix(e, inputs, ids, 2, out, 4);
    TEST_ASSERT(rc == 0, "mix ok");
    TEST_ASSERT(out[0] == 300, "300 = 100+200");

    mix_engine_destroy(e);
    TEST_PASS("mix_engine basic sum");
    return 0;
}

static int test_engine_mix_clip(void) {
    printf("\n=== test_engine_mix_clip ===\n");

    mix_engine_t *e = mix_engine_create();
    mix_source_t s  = make_src(1, 1.0f, false);
    mix_engine_add_source(e, &s);

    /* Source value 30000 with weight 2.0 → 60000 → clipped to 32767 */
    mix_source_set_weight(&s, 2.0f);
    mix_engine_update_source(e, &s);

    int16_t in[2]  = {30000, -30000};
    const int16_t *inputs[1] = {in};
    uint32_t ids[1] = {1};
    int16_t  out[2] = {0};

    mix_engine_mix(e, inputs, ids, 1, out, 2);
    TEST_ASSERT(out[0] ==  32767, "positive clip to +32767");
    TEST_ASSERT(out[1] == -32768, "negative clip to -32768");

    mix_engine_destroy(e);
    TEST_PASS("mix_engine hard-clip");
    return 0;
}

static int test_engine_mix_mute(void) {
    printf("\n=== test_engine_mix_mute ===\n");

    mix_engine_t *e = mix_engine_create();
    mix_source_t s  = make_src(1, 1.0f, true); /* muted */
    mix_engine_add_source(e, &s);

    int16_t in[4] = {1000, 1000, 1000, 1000};
    const int16_t *inputs[1] = {in};
    uint32_t ids[1] = {1};
    int16_t  out[4] = {0};

    mix_engine_mix(e, inputs, ids, 1, out, 4);
    TEST_ASSERT(out[0] == 0, "muted source → silence");

    mix_engine_destroy(e);
    TEST_PASS("mix_engine muted source");
    return 0;
}

static int test_engine_silence(void) {
    printf("\n=== test_engine_silence ===\n");

    int16_t buf[8] = {1,2,3,4,5,6,7,8};
    mix_engine_silence(buf, 8);
    for (int i = 0; i < 8; i++)
        TEST_ASSERT(buf[i] == 0, "buf zeroed");

    TEST_PASS("mix_engine_silence");
    return 0;
}

/* ── mix_stats tests ─────────────────────────────────────────────── */

static int test_mix_stats(void) {
    printf("\n=== test_mix_stats ===\n");

    mix_stats_t *st = mix_stats_create();
    TEST_ASSERT(st != NULL, "created");

    mix_stats_record(st, 2, 1, 500);  /* 2 active, 1 muted, 500µs latency */
    mix_stats_record(st, 0, 0, 300);  /* underrun */
    mix_stats_record(st, 3, 0, 100);

    mix_stats_snapshot_t snap;
    int rc = mix_stats_snapshot(st, &snap);
    TEST_ASSERT(rc == 0, "snapshot ok");
    TEST_ASSERT(snap.mix_calls == 3, "3 calls");
    TEST_ASSERT(snap.active_sources == 5, "5 total active source events");
    TEST_ASSERT(snap.muted_sources  == 1, "1 muted source event");
    TEST_ASSERT(snap.underruns == 1, "1 underrun");
    /* avg = (500+300+100)/3 = 300µs */
    TEST_ASSERT(fabs(snap.avg_latency_us - 300.0) < 1.0, "avg 300µs");
    TEST_ASSERT(fabs(snap.min_latency_us - 100.0) < 1.0, "min 100µs");
    TEST_ASSERT(fabs(snap.max_latency_us - 500.0) < 1.0, "max 500µs");

    mix_stats_reset(st);
    mix_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.mix_calls == 0, "reset ok");

    mix_stats_destroy(st);
    TEST_PASS("mix_stats record/snapshot/underrun/reset");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_source_init();
    failures += test_source_mutate();

    failures += test_engine_add_remove();
    failures += test_engine_mix_basic();
    failures += test_engine_mix_clip();
    failures += test_engine_mix_mute();
    failures += test_engine_silence();

    failures += test_mix_stats();

    printf("\n");
    if (failures == 0)
        printf("ALL MIXER TESTS PASSED\n");
    else
        printf("%d MIXER TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
