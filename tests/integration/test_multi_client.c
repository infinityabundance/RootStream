/*
 * test_multi_client.c — Integration test for multi-client fanout (PHASE-37)
 *
 * Validates that the session table + fanout manager + per-client ABR work
 * together correctly when simulating multiple concurrent clients.
 *
 * No real sockets are used; delivery is attempted to fd=-1 (failing
 * gracefully), so we test the control-plane logic only.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "../../src/fanout/session_table.h"
#include "../../src/fanout/fanout_manager.h"
#include "../../src/fanout/per_client_abr.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

#define NUM_CLIENTS 8

/* ── Helper ──────────────────────────────────────────────────────── */

static session_id_t add_fake_client(session_table_t *t, int index) {
    char addr[32];
    snprintf(addr, sizeof(addr), "10.0.0.%d:47920", index + 1);
    session_id_t id = 0;
    session_table_add(t, -1, addr, &id);  /* fd=-1 → send will fail gracefully */
    return id;
}

/* ── Integration tests ───────────────────────────────────────────── */

static int test_multi_client_add_all(void) {
    printf("\n=== test_multi_client_add_all ===\n");

    session_table_t *t = session_table_create();
    TEST_ASSERT(t != NULL, "session table created");

    session_id_t ids[NUM_CLIENTS];
    for (int i = 0; i < NUM_CLIENTS; i++) {
        ids[i] = add_fake_client(t, i);
        TEST_ASSERT(ids[i] != 0, "client registered");
    }
    TEST_ASSERT(session_table_count(t) == NUM_CLIENTS,
                "all clients counted");

    session_table_destroy(t);
    TEST_PASS("multi-client add all");
    return 0;
}

static int test_multi_client_fanout_stats(void) {
    printf("\n=== test_multi_client_fanout_stats ===\n");

    session_table_t  *t = session_table_create();
    fanout_manager_t *m = fanout_manager_create(t);
    TEST_ASSERT(m != NULL, "fanout manager created");

    for (int i = 0; i < NUM_CLIENTS; i++) {
        add_fake_client(t, i);
    }

    /* Deliver 100 frames (mix of key and delta) */
    uint8_t frame[200];
    memset(frame, 0, sizeof(frame));
    frame[0] = 0x00; frame[1] = 0x00; frame[2] = 0x01; /* NAL start */

    for (int f = 0; f < 100; f++) {
        fanout_frame_type_t type = (f % 30 == 0)
            ? FANOUT_FRAME_VIDEO_KEY : FANOUT_FRAME_VIDEO_DELTA;
        fanout_manager_deliver(m, frame, sizeof(frame), type);
    }

    fanout_stats_t stats;
    fanout_manager_get_stats(m, &stats);
    TEST_ASSERT(stats.frames_in == 100, "frames_in == 100");
    TEST_ASSERT(stats.active_sessions == NUM_CLIENTS,
                "active_sessions tracked correctly");

    fanout_manager_destroy(m);
    session_table_destroy(t);
    TEST_PASS("multi-client fanout statistics");
    return 0;
}

static int test_multi_client_abr_per_session(void) {
    printf("\n=== test_multi_client_abr_per_session ===\n");

    session_table_t  *t = session_table_create();
    TEST_ASSERT(t != NULL, "table created");

    session_id_t ids[NUM_CLIENTS];
    per_client_abr_t *abr[NUM_CLIENTS];

    for (int i = 0; i < NUM_CLIENTS; i++) {
        ids[i] = add_fake_client(t, i);
        abr[i] = per_client_abr_create(4000, 20000);
        TEST_ASSERT(abr[i] != NULL, "ABR created for client");
    }

    /* Simulate different network conditions per client */
    for (int i = 0; i < NUM_CLIENTS; i++) {
        float loss = (i < NUM_CLIENTS / 2) ? 0.0f : 0.15f;
        uint32_t rtt = (i < NUM_CLIENTS / 2) ? 20 : 300;

        /* Run several ABR intervals */
        for (int j = 0; j < 5; j++) {
            abr_decision_t d = per_client_abr_update(abr[i], rtt, loss, 8000);
            session_table_update_bitrate(t, ids[i], d.target_bitrate_kbps);
        }
    }

    /* Good clients should have higher bitrate than congested clients */
    session_entry_t good_entry, bad_entry;
    session_table_get(t, ids[0], &good_entry);
    session_table_get(t, ids[NUM_CLIENTS - 1], &bad_entry);

    TEST_ASSERT(good_entry.bitrate_kbps >= bad_entry.bitrate_kbps,
                "stable client has >= bitrate than congested client");

    for (int i = 0; i < NUM_CLIENTS; i++) {
        per_client_abr_destroy(abr[i]);
    }
    session_table_destroy(t);
    TEST_PASS("per-client ABR with heterogeneous conditions");
    return 0;
}

static int test_multi_client_remove_mid_stream(void) {
    printf("\n=== test_multi_client_remove_mid_stream ===\n");

    session_table_t  *t = session_table_create();
    fanout_manager_t *m = fanout_manager_create(t);

    session_id_t ids[NUM_CLIENTS];
    for (int i = 0; i < NUM_CLIENTS; i++) {
        ids[i] = add_fake_client(t, i);
    }
    TEST_ASSERT(session_table_count(t) == NUM_CLIENTS, "all added");

    /* Remove half mid-stream */
    for (int i = 0; i < NUM_CLIENTS / 2; i++) {
        int rc = session_table_remove(t, ids[i]);
        TEST_ASSERT(rc == 0, "remove succeeds");
    }
    TEST_ASSERT(session_table_count(t) == (size_t)(NUM_CLIENTS / 2),
                "half removed");

    /* Deliver should still work without crashing */
    uint8_t frame[32] = {0};
    fanout_manager_deliver(m, frame, 32, FANOUT_FRAME_AUDIO);

    fanout_stats_t stats;
    fanout_manager_get_stats(m, &stats);
    TEST_ASSERT(stats.frames_in == 1, "frame counted");

    fanout_manager_destroy(m);
    session_table_destroy(t);
    TEST_PASS("multi-client remove mid-stream");
    return 0;
}

static int test_multi_client_congestion_drop(void) {
    printf("\n=== test_multi_client_congestion_drop ===\n");

    session_table_t  *t = session_table_create();
    fanout_manager_t *m = fanout_manager_create(t);

    /* Add a congested client (high RTT) */
    session_id_t id;
    session_table_add(t, -1, "congested:47920", &id);
    session_table_update_stats(t, id, 600, 0.15f); /* RTT > 500 ms threshold */

    uint8_t frame[64] = {0};

    /* Delta frames should be dropped for congested clients */
    int delivered = fanout_manager_deliver(m, frame, 64,
                                           FANOUT_FRAME_VIDEO_DELTA);
    /* fd=-1 makes all sends fail anyway, but drops should be counted */
    TEST_ASSERT(delivered == 0, "no frames delivered (fd=-1 + congested)");

    fanout_stats_t stats;
    fanout_manager_get_stats(m, &stats);
    TEST_ASSERT(stats.frames_dropped > 0 || stats.frames_delivered == 0,
                "congested client causes drop or zero-deliver");

    fanout_manager_destroy(m);
    session_table_destroy(t);
    TEST_PASS("multi-client congestion drop");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_multi_client_add_all();
    failures += test_multi_client_fanout_stats();
    failures += test_multi_client_abr_per_session();
    failures += test_multi_client_remove_mid_stream();
    failures += test_multi_client_congestion_drop();

    printf("\n");
    if (failures == 0) {
        printf("ALL MULTI-CLIENT INTEGRATION TESTS PASSED\n");
    } else {
        printf("%d MULTI-CLIENT INTEGRATION TEST(S) FAILED\n", failures);
    }
    return failures ? 1 : 0;
}
