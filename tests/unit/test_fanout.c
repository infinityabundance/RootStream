/*
 * test_fanout.c — Unit tests for PHASE-37 Multi-Client Fanout
 *
 * Tests session_table, fanout_manager (no sockets), and per_client_abr
 * without requiring real network connections.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* ── session_table tests ─────────────────────────────────────────── */

static int test_session_table_create(void) {
    printf("\n=== test_session_table_create ===\n");

    session_table_t *t = session_table_create();
    TEST_ASSERT(t != NULL, "session table created");
    TEST_ASSERT(session_table_count(t) == 0, "initial count == 0");

    session_table_destroy(t);
    session_table_destroy(NULL); /* must not crash */
    TEST_PASS("session table create/destroy");
    return 0;
}

static int test_session_table_add_remove(void) {
    printf("\n=== test_session_table_add_remove ===\n");

    session_table_t *t = session_table_create();
    TEST_ASSERT(t != NULL, "session table created");

    session_id_t id1, id2;
    int rc = session_table_add(t, 10, "192.168.1.2:47920", &id1);
    TEST_ASSERT(rc == 0, "add session 1 succeeds");
    TEST_ASSERT(id1 >= 1, "id1 >= 1");
    TEST_ASSERT(session_table_count(t) == 1, "count == 1 after add");

    rc = session_table_add(t, 11, "192.168.1.3:47920", &id2);
    TEST_ASSERT(rc == 0, "add session 2 succeeds");
    TEST_ASSERT(id2 != id1, "session IDs are unique");
    TEST_ASSERT(session_table_count(t) == 2, "count == 2 after add");

    /* Retrieve session 1 */
    session_entry_t entry;
    rc = session_table_get(t, id1, &entry);
    TEST_ASSERT(rc == 0, "get session 1 succeeds");
    TEST_ASSERT(entry.socket_fd == 10, "socket_fd matches");
    TEST_ASSERT(strcmp(entry.peer_addr, "192.168.1.2:47920") == 0,
                "peer_addr matches");

    /* Remove session 1 */
    rc = session_table_remove(t, id1);
    TEST_ASSERT(rc == 0, "remove session 1 succeeds");
    TEST_ASSERT(session_table_count(t) == 1, "count == 1 after remove");

    rc = session_table_remove(t, id1);
    TEST_ASSERT(rc == -1, "remove already-removed returns -1");

    /* Get nonexistent */
    rc = session_table_get(t, id1, &entry);
    TEST_ASSERT(rc == -1, "get removed session returns -1");

    session_table_destroy(t);
    TEST_PASS("session table add/remove");
    return 0;
}

static int test_session_table_full(void) {
    printf("\n=== test_session_table_full ===\n");

    session_table_t *t = session_table_create();
    TEST_ASSERT(t != NULL, "session table created");

    session_id_t id;
    /* Fill to capacity */
    for (int i = 0; i < SESSION_TABLE_MAX; i++) {
        char addr[32];
        snprintf(addr, sizeof(addr), "10.0.0.%d:1000", i);
        int rc = session_table_add(t, i + 100, addr, &id);
        TEST_ASSERT(rc == 0, "add within capacity succeeds");
    }
    TEST_ASSERT((int)session_table_count(t) == SESSION_TABLE_MAX,
                "count == SESSION_TABLE_MAX");

    /* One more should fail */
    int rc = session_table_add(t, 999, "overflow:1", &id);
    TEST_ASSERT(rc == -1, "add beyond capacity fails");

    session_table_destroy(t);
    TEST_PASS("session table capacity limit");
    return 0;
}

static int test_session_table_update(void) {
    printf("\n=== test_session_table_update ===\n");

    session_table_t *t = session_table_create();
    TEST_ASSERT(t != NULL, "session table created");

    session_id_t id;
    session_table_add(t, 5, "peer:47920", &id);

    int rc = session_table_update_bitrate(t, id, 8000);
    TEST_ASSERT(rc == 0, "update_bitrate returns 0");

    rc = session_table_update_stats(t, id, 30, 0.01f);
    TEST_ASSERT(rc == 0, "update_stats returns 0");

    session_entry_t entry;
    session_table_get(t, id, &entry);
    TEST_ASSERT(entry.bitrate_kbps == 8000, "bitrate updated");
    TEST_ASSERT(entry.rtt_ms == 30, "rtt_ms updated");

    /* Update nonexistent */
    rc = session_table_update_bitrate(t, 9999, 1000);
    TEST_ASSERT(rc == -1, "update nonexistent returns -1");

    session_table_destroy(t);
    TEST_PASS("session table update");
    return 0;
}

/* Collect visited IDs */
static session_id_t visited_ids[SESSION_TABLE_MAX];
static int          visited_count;

static void collect_ids(const session_entry_t *e, void *ud) {
    (void)ud;
    visited_ids[visited_count++] = e->id;
}

static int test_session_table_foreach(void) {
    printf("\n=== test_session_table_foreach ===\n");

    session_table_t *t = session_table_create();
    TEST_ASSERT(t != NULL, "session table created");

    session_id_t id1, id2, id3;
    session_table_add(t, 1, "a:1", &id1);
    session_table_add(t, 2, "b:1", &id2);
    session_table_add(t, 3, "c:1", &id3);
    session_table_remove(t, id2); /* Remove middle */

    visited_count = 0;
    session_table_foreach(t, collect_ids, NULL);
    TEST_ASSERT(visited_count == 2, "foreach visits 2 active sessions");
    /* Verify id2 not in visited list */
    for (int i = 0; i < visited_count; i++) {
        TEST_ASSERT(visited_ids[i] != id2, "removed session not visited");
    }

    session_table_destroy(t);
    TEST_PASS("session table foreach");
    return 0;
}

/* ── fanout_manager tests (no sockets — fd=-1) ───────────────────── */

static int test_fanout_manager_create(void) {
    printf("\n=== test_fanout_manager_create ===\n");

    session_table_t  *t = session_table_create();
    fanout_manager_t *m = fanout_manager_create(t);
    TEST_ASSERT(m != NULL, "fanout manager created");
    TEST_ASSERT(fanout_manager_create(NULL) == NULL,
                "create NULL table returns NULL");

    fanout_manager_destroy(m);
    fanout_manager_destroy(NULL); /* must not crash */
    session_table_destroy(t);
    TEST_PASS("fanout manager create/destroy");
    return 0;
}

static int test_fanout_manager_deliver_no_sessions(void) {
    printf("\n=== test_fanout_manager_deliver_no_sessions ===\n");

    session_table_t  *t = session_table_create();
    fanout_manager_t *m = fanout_manager_create(t);

    uint8_t frame[64] = {0x00, 0x00, 0x01};
    int delivered = fanout_manager_deliver(m, frame, sizeof(frame),
                                           FANOUT_FRAME_VIDEO_KEY);
    TEST_ASSERT(delivered == 0, "no sessions => 0 delivered");

    fanout_stats_t stats;
    fanout_manager_get_stats(m, &stats);
    TEST_ASSERT(stats.frames_in == 1, "frames_in == 1");
    TEST_ASSERT(stats.frames_delivered == 0, "frames_delivered == 0");

    fanout_manager_destroy(m);
    session_table_destroy(t);
    TEST_PASS("fanout deliver with no sessions");
    return 0;
}

static int test_fanout_manager_stats_reset(void) {
    printf("\n=== test_fanout_manager_stats_reset ===\n");

    session_table_t  *t = session_table_create();
    fanout_manager_t *m = fanout_manager_create(t);

    uint8_t frame[8] = {0};
    fanout_manager_deliver(m, frame, 8, FANOUT_FRAME_AUDIO);
    fanout_manager_deliver(m, frame, 8, FANOUT_FRAME_AUDIO);

    fanout_stats_t stats;
    fanout_manager_get_stats(m, &stats);
    TEST_ASSERT(stats.frames_in == 2, "frames_in == 2 before reset");

    fanout_manager_reset_stats(m);
    fanout_manager_get_stats(m, &stats);
    TEST_ASSERT(stats.frames_in == 0, "frames_in == 0 after reset");

    fanout_manager_destroy(m);
    session_table_destroy(t);
    TEST_PASS("fanout stats reset");
    return 0;
}

/* ── per_client_abr tests ────────────────────────────────────────── */

static int test_abr_create_destroy(void) {
    printf("\n=== test_abr_create_destroy ===\n");

    per_client_abr_t *abr = per_client_abr_create(4000, 20000);
    TEST_ASSERT(abr != NULL, "ABR created");
    TEST_ASSERT(per_client_abr_get_bitrate(abr) == 4000,
                "initial bitrate == 4000");

    per_client_abr_destroy(abr);
    per_client_abr_destroy(NULL); /* must not crash */
    TEST_PASS("per_client_abr create/destroy");
    return 0;
}

static int test_abr_decrease_on_loss(void) {
    printf("\n=== test_abr_decrease_on_loss ===\n");

    per_client_abr_t *abr = per_client_abr_create(8000, 20000);
    TEST_ASSERT(abr != NULL, "ABR created");

    /* High loss should trigger decrease */
    abr_decision_t d = per_client_abr_update(abr, 50, 0.20f, 8000);
    TEST_ASSERT(d.target_bitrate_kbps < 8000,
                "bitrate decreases on 20% loss");
    TEST_ASSERT(d.force_keyframe == true, "keyframe forced on decrease");

    per_client_abr_destroy(abr);
    TEST_PASS("ABR decreases on high packet loss");
    return 0;
}

static int test_abr_increase_when_stable(void) {
    printf("\n=== test_abr_increase_when_stable ===\n");

    per_client_abr_t *abr = per_client_abr_create(4000, 20000);
    TEST_ASSERT(abr != NULL, "ABR created");

    uint32_t rate_before = per_client_abr_get_bitrate(abr);

    /* Two stable intervals → additive increase */
    per_client_abr_update(abr, 20, 0.001f, 10000);
    per_client_abr_update(abr, 20, 0.001f, 10000);

    uint32_t rate_after = per_client_abr_get_bitrate(abr);
    TEST_ASSERT(rate_after > rate_before, "bitrate increases after stable periods");

    per_client_abr_destroy(abr);
    TEST_PASS("ABR increases on stable connection");
    return 0;
}

static int test_abr_max_cap(void) {
    printf("\n=== test_abr_max_cap ===\n");

    per_client_abr_t *abr = per_client_abr_create(9800, 10000);
    TEST_ASSERT(abr != NULL, "ABR created");

    /* Many stable intervals — rate must not exceed max */
    for (int i = 0; i < 50; i++) {
        per_client_abr_update(abr, 10, 0.0f, 50000);
    }
    TEST_ASSERT(per_client_abr_get_bitrate(abr) <= 10000,
                "bitrate capped at max");

    per_client_abr_destroy(abr);
    TEST_PASS("ABR caps at configured maximum");
    return 0;
}

static int test_abr_force_keyframe(void) {
    printf("\n=== test_abr_force_keyframe ===\n");

    per_client_abr_t *abr = per_client_abr_create(4000, 20000);
    TEST_ASSERT(abr != NULL, "ABR created");

    per_client_abr_force_keyframe(abr);

    /* Next update should include force_keyframe */
    abr_decision_t d = per_client_abr_update(abr, 20, 0.0f, 8000);
    TEST_ASSERT(d.force_keyframe == true, "force_keyframe propagated");

    per_client_abr_destroy(abr);
    TEST_PASS("per_client_abr force keyframe");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_session_table_create();
    failures += test_session_table_add_remove();
    failures += test_session_table_full();
    failures += test_session_table_update();
    failures += test_session_table_foreach();

    failures += test_fanout_manager_create();
    failures += test_fanout_manager_deliver_no_sessions();
    failures += test_fanout_manager_stats_reset();

    failures += test_abr_create_destroy();
    failures += test_abr_decrease_on_loss();
    failures += test_abr_increase_when_stable();
    failures += test_abr_max_cap();
    failures += test_abr_force_keyframe();

    printf("\n");
    if (failures == 0) {
        printf("ALL FANOUT TESTS PASSED\n");
    } else {
        printf("%d FANOUT TEST(S) FAILED\n", failures);
    }
    return failures ? 1 : 0;
}
