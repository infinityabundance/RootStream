/*
 * test_session_persist.c — Unit tests for PHASE-41 Session Persistence
 *
 * Tests session_state (serialise/deserialise), session_checkpoint
 * (save/load/delete/exists), and session_resume (encode/decode/evaluate).
 * All I/O uses /tmp; no network connections required.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/session/session_state.h"
#include "../../src/session/session_checkpoint.h"
#include "../../src/session/session_resume.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── Helpers ─────────────────────────────────────────────────────── */

static session_state_t make_state(uint64_t session_id) {
    session_state_t s;
    memset(&s, 0, sizeof(s));
    s.session_id        = session_id;
    s.created_us        = 1234567890ULL;
    s.width             = 1920;
    s.height            = 1080;
    s.fps_num           = 60;
    s.fps_den           = 1;
    s.bitrate_kbps      = 8000;
    s.audio_sample_rate = 48000;
    s.audio_channels    = 2;
    s.last_keyframe     = 150;
    s.frames_sent       = 3600;
    memset(s.stream_key, 0xAB, SESSION_STREAM_KEY_LEN);
    snprintf(s.peer_addr, sizeof(s.peer_addr), "192.168.1.10:47920");
    return s;
}

/* ── session_state tests ─────────────────────────────────────────── */

static int test_state_roundtrip(void) {
    printf("\n=== test_state_roundtrip ===\n");

    session_state_t orig = make_state(42ULL);
    uint8_t buf[SESSION_STATE_MAX_SIZE];
    int n = session_state_serialise(&orig, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "serialise returns positive size");
    TEST_ASSERT((size_t)n == session_state_serialised_size(&orig),
                "size matches predicted size");

    session_state_t decoded;
    int rc = session_state_deserialise(buf, (size_t)n, &decoded);
    TEST_ASSERT(rc == 0, "deserialise succeeds");
    TEST_ASSERT(decoded.session_id        == 42ULL,   "session_id preserved");
    TEST_ASSERT(decoded.width             == 1920,     "width preserved");
    TEST_ASSERT(decoded.height            == 1080,     "height preserved");
    TEST_ASSERT(decoded.fps_num           == 60,       "fps_num preserved");
    TEST_ASSERT(decoded.bitrate_kbps      == 8000,     "bitrate preserved");
    TEST_ASSERT(decoded.audio_sample_rate == 48000,    "sample_rate preserved");
    TEST_ASSERT(decoded.audio_channels    == 2,        "channels preserved");
    TEST_ASSERT(decoded.last_keyframe     == 150,      "last_keyframe preserved");
    TEST_ASSERT(decoded.frames_sent       == 3600,     "frames_sent preserved");
    TEST_ASSERT(strcmp(decoded.peer_addr, "192.168.1.10:47920") == 0,
                "peer_addr preserved");
    TEST_ASSERT(memcmp(decoded.stream_key, orig.stream_key,
                       SESSION_STREAM_KEY_LEN) == 0,
                "stream_key preserved");

    TEST_PASS("session state serialise/deserialise round-trip");
    return 0;
}

static int test_state_bad_magic(void) {
    printf("\n=== test_state_bad_magic ===\n");

    uint8_t buf[SESSION_STATE_MAX_SIZE];
    memset(buf, 0, sizeof(buf));
    buf[0] = 0xFF; buf[1] = 0xFF; buf[2] = 0xFF; buf[3] = 0xFF;

    session_state_t state;
    int rc = session_state_deserialise(buf, sizeof(buf), &state);
    TEST_ASSERT(rc == -1, "bad magic returns -1");

    TEST_PASS("session state rejects bad magic");
    return 0;
}

static int test_state_null_guards(void) {
    printf("\n=== test_state_null_guards ===\n");

    uint8_t buf[SESSION_STATE_MAX_SIZE];
    int n = session_state_serialise(NULL, buf, sizeof(buf));
    TEST_ASSERT(n == -1, "serialise NULL state returns -1");

    session_state_t s = make_state(1);
    n = session_state_serialise(&s, NULL, 0);
    TEST_ASSERT(n == -1, "serialise NULL buf returns -1");

    session_state_t out;
    int rc = session_state_deserialise(NULL, 0, &out);
    TEST_ASSERT(rc == -1, "deserialise NULL buf returns -1");

    TEST_PASS("session state NULL guards");
    return 0;
}

/* ── session_checkpoint tests ────────────────────────────────────── */

static int test_checkpoint_save_load(void) {
    printf("\n=== test_checkpoint_save_load ===\n");

    checkpoint_config_t cfg;
    snprintf(cfg.dir, sizeof(cfg.dir), "/tmp");
    cfg.max_keep = 3;

    checkpoint_manager_t *m = checkpoint_manager_create(&cfg);
    TEST_ASSERT(m != NULL, "checkpoint manager created");

    session_state_t orig = make_state(999ULL);

    /* Clean up any leftover files first */
    checkpoint_delete(m, 999ULL);

    int rc = checkpoint_save(m, &orig);
    TEST_ASSERT(rc == 0, "checkpoint_save returns 0");
    TEST_ASSERT(checkpoint_exists(m, 999ULL), "checkpoint file exists");

    session_state_t loaded;
    rc = checkpoint_load(m, 999ULL, &loaded);
    TEST_ASSERT(rc == 0, "checkpoint_load returns 0");
    TEST_ASSERT(loaded.session_id   == 999ULL, "session_id loaded correctly");
    TEST_ASSERT(loaded.width        == 1920,   "width loaded correctly");
    TEST_ASSERT(loaded.frames_sent  == 3600,   "frames_sent loaded correctly");

    /* Clean up */
    checkpoint_delete(m, 999ULL);
    TEST_ASSERT(!checkpoint_exists(m, 999ULL), "checkpoint deleted");

    checkpoint_manager_destroy(m);
    TEST_PASS("checkpoint save/load/delete");
    return 0;
}

static int test_checkpoint_nonexistent(void) {
    printf("\n=== test_checkpoint_nonexistent ===\n");

    checkpoint_config_t cfg;
    snprintf(cfg.dir, sizeof(cfg.dir), "/tmp");
    cfg.max_keep = 3;
    checkpoint_manager_t *m = checkpoint_manager_create(&cfg);

    session_state_t state;
    int rc = checkpoint_load(m, 0xDEADC0DEULL, &state);
    TEST_ASSERT(rc == -1, "load nonexistent returns -1");
    TEST_ASSERT(!checkpoint_exists(m, 0xDEADC0DEULL),
                "nonexistent session returns false");

    checkpoint_manager_destroy(m);
    TEST_PASS("checkpoint non-existent session");
    return 0;
}

static int test_checkpoint_null(void) {
    printf("\n=== test_checkpoint_null ===\n");

    checkpoint_manager_t *m = checkpoint_manager_create(NULL);
    TEST_ASSERT(m != NULL, "manager created with NULL config (defaults)");
    checkpoint_manager_destroy(m);
    checkpoint_manager_destroy(NULL); /* must not crash */

    TEST_PASS("checkpoint manager NULL config / destroy(NULL)");
    return 0;
}

/* ── session_resume tests ────────────────────────────────────────── */

static int test_resume_request_roundtrip(void) {
    printf("\n=== test_resume_request_roundtrip ===\n");

    resume_request_t req;
    req.session_id           = 77ULL;
    req.last_frame_received  = 350ULL;
    memset(req.stream_key, 0xCC, SESSION_STREAM_KEY_LEN);

    uint8_t buf[128];
    int n = resume_encode_request(&req, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "encode_request returns positive size");

    resume_request_t decoded;
    int rc = resume_decode_request(buf, (size_t)n, &decoded);
    TEST_ASSERT(rc == 0, "decode_request succeeds");
    TEST_ASSERT(decoded.session_id          == 77ULL,  "session_id preserved");
    TEST_ASSERT(decoded.last_frame_received == 350ULL, "last_frame preserved");
    TEST_ASSERT(memcmp(decoded.stream_key, req.stream_key,
                       SESSION_STREAM_KEY_LEN) == 0,
                "stream_key preserved");

    TEST_PASS("resume request encode/decode round-trip");
    return 0;
}

static int test_resume_accepted_roundtrip(void) {
    printf("\n=== test_resume_accepted_roundtrip ===\n");

    resume_accepted_t acc = { 100ULL, 120, 6000 };
    uint8_t buf[64];
    int n = resume_encode_accepted(&acc, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "encode_accepted positive");

    resume_accepted_t decoded;
    int rc = resume_decode_accepted(buf, (size_t)n, &decoded);
    TEST_ASSERT(rc == 0, "decode_accepted succeeds");
    TEST_ASSERT(decoded.session_id        == 100ULL, "session_id preserved");
    TEST_ASSERT(decoded.resume_from_frame == 120,    "resume_from_frame preserved");
    TEST_ASSERT(decoded.bitrate_kbps      == 6000,   "bitrate preserved");

    TEST_PASS("resume accepted encode/decode round-trip");
    return 0;
}

static int test_resume_rejected_roundtrip(void) {
    printf("\n=== test_resume_rejected_roundtrip ===\n");

    resume_rejected_t rej = { 55ULL, RESUME_REJECT_FRAME_GAP_TOO_LARGE };
    uint8_t buf[64];
    int n = resume_encode_rejected(&rej, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "encode_rejected positive");

    resume_rejected_t decoded;
    int rc = resume_decode_rejected(buf, (size_t)n, &decoded);
    TEST_ASSERT(rc == 0, "decode_rejected succeeds");
    TEST_ASSERT(decoded.session_id == 55ULL, "session_id preserved");
    TEST_ASSERT(decoded.reason == RESUME_REJECT_FRAME_GAP_TOO_LARGE,
                "reason preserved");

    TEST_PASS("resume rejected encode/decode round-trip");
    return 0;
}

static int test_resume_server_accept(void) {
    printf("\n=== test_resume_server_accept ===\n");

    session_state_t srv = make_state(42ULL);
    srv.frames_sent    = 1000;
    srv.last_keyframe  = 960;

    resume_request_t req;
    req.session_id          = 42ULL;
    req.last_frame_received = 990ULL; /* close to server: gap 10 */
    memcpy(req.stream_key, srv.stream_key, SESSION_STREAM_KEY_LEN);

    resume_accepted_t acc;
    resume_rejected_t rej;
    bool ok = resume_server_evaluate(&req, &srv, 100, &acc, &rej);
    TEST_ASSERT(ok, "server accepts valid resume request");
    TEST_ASSERT(acc.resume_from_frame == 960, "resume from last keyframe");
    TEST_ASSERT(acc.bitrate_kbps == 8000, "bitrate from state");

    TEST_PASS("resume server evaluation: accept");
    return 0;
}

static int test_resume_server_reject_gap(void) {
    printf("\n=== test_resume_server_reject_gap ===\n");

    session_state_t srv = make_state(42ULL);
    srv.frames_sent = 1000;

    resume_request_t req;
    req.session_id          = 42ULL;
    req.last_frame_received = 500ULL; /* gap 500 > max 100 */
    memcpy(req.stream_key, srv.stream_key, SESSION_STREAM_KEY_LEN);

    resume_rejected_t rej;
    bool ok = resume_server_evaluate(&req, &srv, 100, NULL, &rej);
    TEST_ASSERT(!ok, "server rejects large frame gap");
    TEST_ASSERT(rej.reason == RESUME_REJECT_FRAME_GAP_TOO_LARGE,
                "reason FRAME_GAP_TOO_LARGE");

    TEST_PASS("resume server evaluation: reject gap");
    return 0;
}

static int test_resume_server_reject_key_mismatch(void) {
    printf("\n=== test_resume_server_reject_key_mismatch ===\n");

    session_state_t srv = make_state(42ULL);
    srv.frames_sent = 1000;

    resume_request_t req;
    req.session_id          = 42ULL;
    req.last_frame_received = 990ULL;
    memset(req.stream_key, 0x00, SESSION_STREAM_KEY_LEN); /* wrong key */

    resume_rejected_t rej;
    bool ok = resume_server_evaluate(&req, &srv, 100, NULL, &rej);
    TEST_ASSERT(!ok, "server rejects wrong stream key");
    TEST_ASSERT(rej.reason == RESUME_REJECT_STATE_MISMATCH,
                "reason STATE_MISMATCH");

    TEST_PASS("resume server evaluation: reject key mismatch");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_state_roundtrip();
    failures += test_state_bad_magic();
    failures += test_state_null_guards();

    failures += test_checkpoint_save_load();
    failures += test_checkpoint_nonexistent();
    failures += test_checkpoint_null();

    failures += test_resume_request_roundtrip();
    failures += test_resume_accepted_roundtrip();
    failures += test_resume_rejected_roundtrip();
    failures += test_resume_server_accept();
    failures += test_resume_server_reject_gap();
    failures += test_resume_server_reject_key_mismatch();

    printf("\n");
    if (failures == 0)
        printf("ALL SESSION PERSISTENCE TESTS PASSED\n");
    else
        printf("%d SESSION PERSISTENCE TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
