/*
 * test_analytics.c — Unit tests for PHASE-45 Viewer Analytics & Telemetry
 *
 * Tests analytics_event (encode/decode/type_name), event_ring
 * (push/pop/drain/overflow), analytics_stats (ingest/snapshot/reset),
 * and analytics_export (JSON/CSV).  No network or stream hardware required.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/analytics/analytics_event.h"
#include "../../src/analytics/event_ring.h"
#include "../../src/analytics/analytics_stats.h"
#include "../../src/analytics/analytics_export.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── analytics_event tests ───────────────────────────────────────── */

static analytics_event_t make_event(analytics_event_type_t type,
                                     uint64_t timestamp_us,
                                     uint64_t session_id,
                                     uint64_t value,
                                     const char *payload) {
    analytics_event_t e;
    memset(&e, 0, sizeof(e));
    e.type         = type;
    e.timestamp_us = timestamp_us;
    e.session_id   = session_id;
    e.value        = value;
    if (payload) {
        e.payload_len = (uint16_t)strlen(payload);
        snprintf(e.payload, sizeof(e.payload), "%s", payload);
    }
    return e;
}

static int test_event_roundtrip(void) {
    printf("\n=== test_event_roundtrip ===\n");

    analytics_event_t orig = make_event(ANALYTICS_VIEWER_JOIN,
                                         1700000000000000ULL,
                                         42, 0, "viewer1");
    uint8_t buf[ANALYTICS_HDR_SIZE + ANALYTICS_MAX_PAYLOAD + 8];
    int n = analytics_event_encode(&orig, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "encode positive");
    TEST_ASSERT((size_t)n == analytics_event_encoded_size(&orig), "size match");

    analytics_event_t decoded;
    int rc = analytics_event_decode(buf, (size_t)n, &decoded);
    TEST_ASSERT(rc == 0, "decode ok");
    TEST_ASSERT(decoded.type == ANALYTICS_VIEWER_JOIN, "type preserved");
    TEST_ASSERT(decoded.timestamp_us == 1700000000000000ULL, "ts preserved");
    TEST_ASSERT(decoded.session_id == 42, "session_id preserved");
    TEST_ASSERT(strcmp(decoded.payload, "viewer1") == 0, "payload preserved");

    TEST_PASS("analytics event encode/decode round-trip");
    return 0;
}

static int test_event_bad_magic(void) {
    printf("\n=== test_event_bad_magic ===\n");

    uint8_t buf[ANALYTICS_HDR_SIZE] = {0};
    analytics_event_t e;
    int rc = analytics_event_decode(buf, sizeof(buf), &e);
    TEST_ASSERT(rc == -1, "bad magic returns -1");

    TEST_PASS("analytics event bad magic rejected");
    return 0;
}

static int test_event_type_name(void) {
    printf("\n=== test_event_type_name ===\n");

    TEST_ASSERT(strcmp(analytics_event_type_name(ANALYTICS_VIEWER_JOIN),
                       "viewer_join") == 0, "viewer_join name");
    TEST_ASSERT(strcmp(analytics_event_type_name(ANALYTICS_STREAM_START),
                       "stream_start") == 0, "stream_start name");
    TEST_ASSERT(strcmp(analytics_event_type_name(
                       (analytics_event_type_t)0xFF), "unknown") == 0,
                "unknown type");

    TEST_PASS("analytics event type names");
    return 0;
}

static int test_event_null_guards(void) {
    printf("\n=== test_event_null_guards ===\n");

    uint8_t buf[64];
    analytics_event_t e; memset(&e, 0, sizeof(e));
    TEST_ASSERT(analytics_event_encode(NULL, buf, sizeof(buf)) == -1,
                "encode NULL event");
    TEST_ASSERT(analytics_event_encode(&e, NULL, 0) == -1,
                "encode NULL buf");
    TEST_ASSERT(analytics_event_decode(NULL, 0, &e) == -1,
                "decode NULL buf");

    TEST_PASS("analytics event NULL guards");
    return 0;
}

/* ── event_ring tests ────────────────────────────────────────────── */

static int test_ring_create(void) {
    printf("\n=== test_ring_create ===\n");

    event_ring_t *r = event_ring_create();
    TEST_ASSERT(r != NULL, "ring created");
    TEST_ASSERT(event_ring_count(r) == 0, "initial count 0");
    TEST_ASSERT(event_ring_is_empty(r), "initial is_empty");

    event_ring_destroy(r);
    event_ring_destroy(NULL);

    TEST_PASS("event_ring create/destroy");
    return 0;
}

static int test_ring_push_pop(void) {
    printf("\n=== test_ring_push_pop ===\n");

    event_ring_t *r = event_ring_create();
    analytics_event_t e = make_event(ANALYTICS_BITRATE_CHANGE, 100, 1, 2000, "");
    int rc = event_ring_push(r, &e);
    TEST_ASSERT(rc == 0, "push returns 0");
    TEST_ASSERT(event_ring_count(r) == 1, "count 1");

    analytics_event_t out;
    rc = event_ring_peek(r, &out);
    TEST_ASSERT(rc == 0, "peek returns 0");
    TEST_ASSERT(out.value == 2000, "peek value correct");
    TEST_ASSERT(event_ring_count(r) == 1, "count still 1 after peek");

    rc = event_ring_pop(r, &out);
    TEST_ASSERT(rc == 0, "pop returns 0");
    TEST_ASSERT(out.value == 2000, "pop value correct");
    TEST_ASSERT(event_ring_count(r) == 0, "count 0 after pop");

    rc = event_ring_pop(r, &out);
    TEST_ASSERT(rc == -1, "pop empty returns -1");

    event_ring_destroy(r);
    TEST_PASS("event_ring push/pop");
    return 0;
}

static int test_ring_overflow(void) {
    printf("\n=== test_ring_overflow ===\n");

    event_ring_t *r = event_ring_create();
    /* Fill ring past capacity */
    for (int i = 0; i < EVENT_RING_CAPACITY + 5; i++) {
        analytics_event_t e = make_event(ANALYTICS_FRAME_DROP,
                                          (uint64_t)i, 1, (uint64_t)i, "");
        event_ring_push(r, &e);
    }
    TEST_ASSERT(event_ring_count(r) == EVENT_RING_CAPACITY,
                "overflow: count stays at capacity");

    /* Oldest events (0..4) should have been dropped; first valid = 5 */
    analytics_event_t out;
    event_ring_pop(r, &out);
    TEST_ASSERT(out.value == 5, "oldest event after overflow is idx 5");

    event_ring_destroy(r);
    TEST_PASS("event_ring overflow head-drop");
    return 0;
}

static int test_ring_drain(void) {
    printf("\n=== test_ring_drain ===\n");

    event_ring_t *r = event_ring_create();
    for (int i = 0; i < 5; i++) {
        analytics_event_t e = make_event(ANALYTICS_LATENCY_SAMPLE,
                                          (uint64_t)i, 0, (uint64_t)i, "");
        event_ring_push(r, &e);
    }

    analytics_event_t out[3];
    size_t n = event_ring_drain(r, out, 3);
    TEST_ASSERT(n == 3, "drain 3 events");
    TEST_ASSERT(event_ring_count(r) == 2, "2 remaining");
    TEST_ASSERT(out[0].value == 0, "drained[0] correct");
    TEST_ASSERT(out[2].value == 2, "drained[2] correct");

    event_ring_clear(r);
    TEST_ASSERT(event_ring_is_empty(r), "clear empties ring");

    event_ring_destroy(r);
    TEST_PASS("event_ring drain + clear");
    return 0;
}

/* ── analytics_stats tests ───────────────────────────────────────── */

static int test_stats_viewer_counts(void) {
    printf("\n=== test_stats_viewer_counts ===\n");

    analytics_stats_ctx_t *ctx = analytics_stats_create();
    TEST_ASSERT(ctx != NULL, "stats created");

    analytics_event_t e;

    /* 3 viewers join */
    for (int i = 0; i < 3; i++) {
        e = make_event(ANALYTICS_VIEWER_JOIN, (uint64_t)i, (uint64_t)i, 0, "");
        analytics_stats_ingest(ctx, &e);
    }
    analytics_stats_t snap;
    analytics_stats_snapshot(ctx, &snap);
    TEST_ASSERT(snap.total_viewer_joins == 3, "3 joins");
    TEST_ASSERT(snap.current_viewers == 3, "3 concurrent");
    TEST_ASSERT(snap.peak_viewers == 3, "peak 3");

    /* 1 viewer leaves */
    e = make_event(ANALYTICS_VIEWER_LEAVE, 10, 0, 0, "");
    analytics_stats_ingest(ctx, &e);
    analytics_stats_snapshot(ctx, &snap);
    TEST_ASSERT(snap.current_viewers == 2, "current 2 after leave");
    TEST_ASSERT(snap.peak_viewers == 3, "peak unchanged at 3");

    analytics_stats_destroy(ctx);
    TEST_PASS("analytics stats viewer counts");
    return 0;
}

static int test_stats_latency_avg(void) {
    printf("\n=== test_stats_latency_avg ===\n");

    analytics_stats_ctx_t *ctx = analytics_stats_create();
    analytics_event_t e;

    /* Add 4 latency samples: 100, 200, 300, 400 µs → avg = 250 */
    uint64_t latencies[] = {100, 200, 300, 400};
    for (int i = 0; i < 4; i++) {
        e = make_event(ANALYTICS_LATENCY_SAMPLE, (uint64_t)i, 0,
                        latencies[i], "");
        analytics_stats_ingest(ctx, &e);
    }

    analytics_stats_t snap;
    analytics_stats_snapshot(ctx, &snap);
    TEST_ASSERT(fabs(snap.avg_latency_us - 250.0) < 0.001, "avg latency 250µs");

    analytics_stats_reset(ctx);
    analytics_stats_snapshot(ctx, &snap);
    TEST_ASSERT(snap.avg_latency_us == 0.0, "reset clears avg latency");

    analytics_stats_destroy(ctx);
    TEST_PASS("analytics stats latency running average");
    return 0;
}

static int test_stats_stream_events(void) {
    printf("\n=== test_stats_stream_events ===\n");

    analytics_stats_ctx_t *ctx = analytics_stats_create();
    analytics_event_t e;

    e = make_event(ANALYTICS_STREAM_START, 5000, 0, 0, "");
    analytics_stats_ingest(ctx, &e);

    for (int i = 0; i < 3; i++) {
        e = make_event(ANALYTICS_SCENE_CHANGE, (uint64_t)(5100+i), 0, 0, "");
        analytics_stats_ingest(ctx, &e);
    }
    e = make_event(ANALYTICS_QUALITY_ALERT, 5200, 0, 0, "");
    analytics_stats_ingest(ctx, &e);
    e = make_event(ANALYTICS_FRAME_DROP, 5300, 0, 15, "");
    analytics_stats_ingest(ctx, &e);

    analytics_stats_t snap;
    analytics_stats_snapshot(ctx, &snap);
    TEST_ASSERT(snap.stream_start_us == 5000, "stream start ts");
    TEST_ASSERT(snap.scene_changes == 3, "3 scene changes");
    TEST_ASSERT(snap.quality_alerts == 1, "1 quality alert");
    TEST_ASSERT(snap.total_frame_drops == 15, "15 frame drops");

    /* stream stop resets scene_changes counter in next start */
    e = make_event(ANALYTICS_STREAM_START, 9000, 0, 0, "");
    analytics_stats_ingest(ctx, &e);
    analytics_stats_snapshot(ctx, &snap);
    TEST_ASSERT(snap.scene_changes == 0, "scene_changes reset on stream_start");

    analytics_stats_destroy(ctx);
    TEST_PASS("analytics stats stream events");
    return 0;
}

/* ── analytics_export tests ──────────────────────────────────────── */

static int test_export_stats_json(void) {
    printf("\n=== test_export_stats_json ===\n");

    analytics_stats_ctx_t *ctx = analytics_stats_create();
    analytics_event_t e = make_event(ANALYTICS_VIEWER_JOIN, 100, 1, 0, "");
    analytics_stats_ingest(ctx, &e);

    analytics_stats_t snap;
    analytics_stats_snapshot(ctx, &snap);

    char buf[4096];
    int n = analytics_export_stats_json(&snap, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "stats JSON positive");
    TEST_ASSERT(strstr(buf, "\"total_viewer_joins\":1") != NULL,
                "joins in JSON");
    TEST_ASSERT(buf[0] == '{', "starts with {");
    TEST_ASSERT(buf[n-1] == '}', "ends with }");

    /* Too-small buffer */
    n = analytics_export_stats_json(&snap, buf, 4);
    TEST_ASSERT(n == -1, "too-small buffer returns -1");

    analytics_stats_destroy(ctx);
    TEST_PASS("analytics export stats JSON");
    return 0;
}

static int test_export_events_json(void) {
    printf("\n=== test_export_events_json ===\n");

    analytics_event_t events[2];
    events[0] = make_event(ANALYTICS_VIEWER_JOIN,  100, 1, 0, "alice");
    events[1] = make_event(ANALYTICS_VIEWER_LEAVE, 200, 1, 0, "");

    char buf[4096];
    int n = analytics_export_events_json(events, 2, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "events JSON positive");
    TEST_ASSERT(buf[0] == '[', "starts with [");
    TEST_ASSERT(strstr(buf, "viewer_join") != NULL, "type name in JSON");
    TEST_ASSERT(strstr(buf, "alice") != NULL, "payload in JSON");

    /* Empty array */
    n = analytics_export_events_json(events, 0, buf, sizeof(buf));
    TEST_ASSERT(n == 2, "empty array is []");
    TEST_ASSERT(strcmp(buf, "[]") == 0, "empty array content");

    TEST_PASS("analytics export events JSON");
    return 0;
}

static int test_export_events_csv(void) {
    printf("\n=== test_export_events_csv ===\n");

    analytics_event_t events[2];
    events[0] = make_event(ANALYTICS_BITRATE_CHANGE, 1000, 0, 4000, "");
    events[1] = make_event(ANALYTICS_FRAME_DROP,     2000, 0, 3, "");

    char buf[4096];
    int n = analytics_export_events_csv(events, 2, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "events CSV positive");
    TEST_ASSERT(strstr(buf, "timestamp_us,type") != NULL, "CSV header");
    TEST_ASSERT(strstr(buf, "bitrate_change") != NULL, "type in CSV");
    TEST_ASSERT(strstr(buf, "4000") != NULL, "value in CSV");

    TEST_PASS("analytics export events CSV");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_event_roundtrip();
    failures += test_event_bad_magic();
    failures += test_event_type_name();
    failures += test_event_null_guards();

    failures += test_ring_create();
    failures += test_ring_push_pop();
    failures += test_ring_overflow();
    failures += test_ring_drain();

    failures += test_stats_viewer_counts();
    failures += test_stats_latency_avg();
    failures += test_stats_stream_events();

    failures += test_export_stats_json();
    failures += test_export_events_json();
    failures += test_export_events_csv();

    printf("\n");
    if (failures == 0)
        printf("ALL ANALYTICS TESTS PASSED\n");
    else
        printf("%d ANALYTICS TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
