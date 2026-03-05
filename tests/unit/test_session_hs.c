/*
 * test_session_hs.c — Unit tests for PHASE-55 Session Handshake Protocol
 *
 * Tests hs_message (encode/decode/CRC/bad-magic/type-names),
 * hs_state (client FSM / server FSM / error / BYE / state-names),
 * hs_token (from-seed/equal/zero/hex round-trip), and
 * hs_stats (begin/complete/fail/timeout/snapshot/reset).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/session_hs/hs_message.h"
#include "../../src/session_hs/hs_state.h"
#include "../../src/session_hs/hs_token.h"
#include "../../src/session_hs/hs_stats.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", (msg)); return 1; } } while (0)
#define TEST_PASS(msg)  printf("PASS: %s\n", (msg))

/* ── hs_message tests ────────────────────────────────────────────── */

static int test_msg_roundtrip(void) {
    printf("\n=== test_msg_roundtrip ===\n");

    hs_message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type        = HS_MSG_HELLO;
    msg.seq         = 7;
    msg.payload_len = 5;
    memcpy(msg.payload, "hello", 5);

    uint8_t buf[HS_MSG_HDR_SIZE + 256];
    int n = hs_message_encode(&msg, buf, sizeof(buf));
    TEST_ASSERT(n == HS_MSG_HDR_SIZE + 5, "encoded size");

    hs_message_t dec;
    int rc = hs_message_decode(buf, (size_t)n, &dec);
    TEST_ASSERT(rc == 0, "decode ok");
    TEST_ASSERT(dec.type == HS_MSG_HELLO, "type");
    TEST_ASSERT(dec.seq  == 7,            "seq");
    TEST_ASSERT(dec.payload_len == 5,     "payload_len");
    TEST_ASSERT(memcmp(dec.payload, "hello", 5) == 0, "payload data");

    TEST_PASS("hs_message encode/decode round-trip");
    return 0;
}

static int test_msg_crc_tamper(void) {
    printf("\n=== test_msg_crc_tamper ===\n");

    hs_message_t msg; memset(&msg, 0, sizeof(msg));
    msg.type = HS_MSG_READY; msg.seq = 1;

    uint8_t buf[HS_MSG_HDR_SIZE];
    hs_message_encode(&msg, buf, sizeof(buf));
    buf[HS_MSG_HDR_SIZE - 1] ^= 0xFF; /* corrupt last CRC byte */

    hs_message_t dec;
    TEST_ASSERT(hs_message_decode(buf, sizeof(buf), &dec) == -1, "CRC tamper → -1");

    TEST_PASS("hs_message CRC tamper detected");
    return 0;
}

static int test_msg_bad_magic(void) {
    printf("\n=== test_msg_bad_magic ===\n");

    uint8_t buf[HS_MSG_HDR_SIZE] = {0};
    hs_message_t dec;
    TEST_ASSERT(hs_message_decode(buf, sizeof(buf), &dec) == -1, "bad magic → -1");

    TEST_PASS("hs_message bad magic rejected");
    return 0;
}

static int test_msg_type_names(void) {
    printf("\n=== test_msg_type_names ===\n");

    TEST_ASSERT(strcmp(hs_msg_type_name(HS_MSG_HELLO),     "HELLO")     == 0, "HELLO");
    TEST_ASSERT(strcmp(hs_msg_type_name(HS_MSG_HELLO_ACK), "HELLO_ACK") == 0, "HELLO_ACK");
    TEST_ASSERT(strcmp(hs_msg_type_name(HS_MSG_READY),     "READY")     == 0, "READY");
    TEST_ASSERT(strcmp(hs_msg_type_name((hs_msg_type_t)99), "UNKNOWN") == 0,  "unknown");

    TEST_PASS("hs_message type names");
    return 0;
}

/* ── hs_state tests ──────────────────────────────────────────────── */

static hs_message_t make_msg(hs_msg_type_t t) {
    hs_message_t m; memset(&m, 0, sizeof(m)); m.type = t; return m;
}

static int test_fsm_client(void) {
    printf("\n=== test_fsm_client ===\n");

    hs_fsm_t *fsm = hs_fsm_create(HS_ROLE_CLIENT);
    TEST_ASSERT(fsm != NULL, "fsm created");
    TEST_ASSERT(hs_fsm_state(fsm) == HS_ST_INIT, "initial INIT");

    hs_message_t m;
    m = make_msg(HS_MSG_HELLO);    hs_fsm_process(fsm, &m);
    TEST_ASSERT(hs_fsm_state(fsm) == HS_ST_HELLO_SENT, "HELLO → HELLO_SENT");

    m = make_msg(HS_MSG_HELLO_ACK); hs_fsm_process(fsm, &m);
    TEST_ASSERT(hs_fsm_state(fsm) == HS_ST_AUTH, "HELLO_ACK → AUTH");

    m = make_msg(HS_MSG_AUTH);     hs_fsm_process(fsm, &m);
    TEST_ASSERT(hs_fsm_state(fsm) == HS_ST_AUTH_SENT, "AUTH → AUTH_SENT");

    m = make_msg(HS_MSG_AUTH_ACK); hs_fsm_process(fsm, &m);
    TEST_ASSERT(hs_fsm_state(fsm) == HS_ST_CONFIG_WAIT, "AUTH_ACK → CONFIG_WAIT");

    m = make_msg(HS_MSG_CONFIG);   hs_fsm_process(fsm, &m);
    TEST_ASSERT(hs_fsm_state(fsm) == HS_ST_READY, "CONFIG → READY");
    TEST_ASSERT(!hs_fsm_is_terminal(fsm), "READY not terminal");

    hs_fsm_destroy(fsm);
    TEST_PASS("hs_fsm client path");
    return 0;
}

static int test_fsm_server(void) {
    printf("\n=== test_fsm_server ===\n");

    hs_fsm_t *fsm = hs_fsm_create(HS_ROLE_SERVER);

    hs_message_t m;
    m = make_msg(HS_MSG_HELLO);     hs_fsm_process(fsm, &m);
    TEST_ASSERT(hs_fsm_state(fsm) == HS_ST_HELLO_RCVD, "HELLO → HELLO_RCVD");

    m = make_msg(HS_MSG_HELLO_ACK); hs_fsm_process(fsm, &m);
    TEST_ASSERT(hs_fsm_state(fsm) == HS_ST_AUTH_WAIT, "HELLO_ACK → AUTH_WAIT");

    m = make_msg(HS_MSG_AUTH);      hs_fsm_process(fsm, &m);
    TEST_ASSERT(hs_fsm_state(fsm) == HS_ST_AUTH_VERIFY, "AUTH → AUTH_VERIFY");

    m = make_msg(HS_MSG_AUTH_ACK);  hs_fsm_process(fsm, &m);
    TEST_ASSERT(hs_fsm_state(fsm) == HS_ST_CONFIG_SENT, "AUTH_ACK → CONFIG_SENT");

    m = make_msg(HS_MSG_CONFIG);    hs_fsm_process(fsm, &m);
    TEST_ASSERT(hs_fsm_state(fsm) == HS_ST_READY, "CONFIG → READY");

    hs_fsm_destroy(fsm);
    TEST_PASS("hs_fsm server path");
    return 0;
}

static int test_fsm_error_and_bye(void) {
    printf("\n=== test_fsm_error_and_bye ===\n");

    hs_fsm_t *fsm = hs_fsm_create(HS_ROLE_CLIENT);
    hs_fsm_set_error(fsm, 42);
    TEST_ASSERT(hs_fsm_state(fsm) == HS_ST_ERROR, "set_error → ERROR");
    TEST_ASSERT(hs_fsm_is_terminal(fsm), "ERROR is terminal");
    hs_fsm_destroy(fsm);

    fsm = hs_fsm_create(HS_ROLE_SERVER);
    hs_fsm_close(fsm);
    TEST_ASSERT(hs_fsm_state(fsm) == HS_ST_CLOSED, "close → CLOSED");
    TEST_ASSERT(hs_fsm_is_terminal(fsm), "CLOSED is terminal");
    hs_fsm_destroy(fsm);

    TEST_PASS("hs_fsm error and BYE");
    return 0;
}

static int test_fsm_state_names(void) {
    printf("\n=== test_fsm_state_names ===\n");

    TEST_ASSERT(strcmp(hs_state_name(HS_ST_INIT),    "INIT")  == 0, "INIT");
    TEST_ASSERT(strcmp(hs_state_name(HS_ST_READY),   "READY") == 0, "READY");
    TEST_ASSERT(strcmp(hs_state_name(HS_ST_ERROR),   "ERROR") == 0, "ERROR");
    TEST_ASSERT(strcmp(hs_state_name(HS_ST_CLOSED),  "CLOSED")== 0, "CLOSED");
    TEST_ASSERT(strcmp(hs_state_name((hs_state_t)99),"UNKNOWN")== 0,"unknown");

    TEST_PASS("hs_state names");
    return 0;
}

/* ── hs_token tests ──────────────────────────────────────────────── */

static int test_token_from_seed(void) {
    printf("\n=== test_token_from_seed ===\n");

    uint8_t seed[HS_TOKEN_SIZE] = {
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10
    };
    hs_token_t tok;
    TEST_ASSERT(hs_token_from_seed(seed, HS_TOKEN_SIZE, &tok) == 0, "from_seed ok");
    TEST_ASSERT(!hs_token_zero(&tok), "non-zero token");

    /* Same seed → same token */
    hs_token_t tok2;
    hs_token_from_seed(seed, HS_TOKEN_SIZE, &tok2);
    TEST_ASSERT(hs_token_equal(&tok, &tok2), "deterministic");

    /* Different seed → different token */
    seed[0] ^= 0xFF;
    hs_token_t tok3;
    hs_token_from_seed(seed, HS_TOKEN_SIZE, &tok3);
    TEST_ASSERT(!hs_token_equal(&tok, &tok3), "different seed → different token");

    TEST_PASS("hs_token from_seed");
    return 0;
}

static int test_token_hex_roundtrip(void) {
    printf("\n=== test_token_hex_roundtrip ===\n");

    uint8_t seed[HS_TOKEN_SIZE];
    for (int i = 0; i < HS_TOKEN_SIZE; i++) seed[i] = (uint8_t)(i * 17 + 3);
    hs_token_t orig; hs_token_from_seed(seed, HS_TOKEN_SIZE, &orig);

    char hex[33];
    TEST_ASSERT(hs_token_to_hex(&orig, hex, sizeof(hex)) == 0, "to_hex ok");
    TEST_ASSERT(strlen(hex) == 32, "hex length 32");

    hs_token_t parsed;
    TEST_ASSERT(hs_token_from_hex(hex, &parsed) == 0, "from_hex ok");
    TEST_ASSERT(hs_token_equal(&orig, &parsed), "round-trip equal");

    /* Invalid hex */
    TEST_ASSERT(hs_token_from_hex("ZZZZ", &parsed) == -1, "invalid hex → -1");

    TEST_PASS("hs_token hex round-trip");
    return 0;
}

static int test_token_zero(void) {
    printf("\n=== test_token_zero ===\n");

    hs_token_t t; memset(&t, 0, sizeof(t));
    TEST_ASSERT(hs_token_zero(&t), "all-zero → zero");
    t.bytes[0] = 1;
    TEST_ASSERT(!hs_token_zero(&t), "non-zero → not zero");
    TEST_ASSERT(hs_token_zero(NULL), "NULL → zero");

    TEST_PASS("hs_token zero check");
    return 0;
}

/* ── hs_stats tests ──────────────────────────────────────────────── */

static int test_hs_stats(void) {
    printf("\n=== test_hs_stats ===\n");

    hs_stats_t *st = hs_stats_create();
    TEST_ASSERT(st != NULL, "stats created");

    hs_stats_begin(st, 1000);
    hs_stats_complete(st, 5000);  /* RTT = 4000µs */
    hs_stats_begin(st, 6000);
    hs_stats_complete(st, 8000);  /* RTT = 2000µs */
    hs_stats_begin(st, 9000);
    hs_stats_fail(st);
    hs_stats_begin(st, 10000);
    hs_stats_timeout(st);

    hs_stats_snapshot_t snap;
    int rc = hs_stats_snapshot(st, &snap);
    TEST_ASSERT(rc == 0, "snapshot ok");
    TEST_ASSERT(snap.attempts   == 4, "4 attempts");
    TEST_ASSERT(snap.successes  == 2, "2 successes");
    TEST_ASSERT(snap.failures   == 1, "1 failure");
    TEST_ASSERT(snap.timeouts   == 1, "1 timeout");
    TEST_ASSERT(fabs(snap.avg_rtt_us - 3000.0) < 1.0, "avg RTT 3000µs");
    TEST_ASSERT(fabs(snap.min_rtt_us - 2000.0) < 1.0, "min RTT 2000µs");
    TEST_ASSERT(fabs(snap.max_rtt_us - 4000.0) < 1.0, "max RTT 4000µs");

    hs_stats_reset(st);
    hs_stats_snapshot(st, &snap);
    TEST_ASSERT(snap.attempts == 0, "reset clears attempts");

    hs_stats_destroy(st);
    TEST_PASS("hs_stats begin/complete/fail/timeout/snapshot/reset");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_msg_roundtrip();
    failures += test_msg_crc_tamper();
    failures += test_msg_bad_magic();
    failures += test_msg_type_names();

    failures += test_fsm_client();
    failures += test_fsm_server();
    failures += test_fsm_error_and_bye();
    failures += test_fsm_state_names();

    failures += test_token_from_seed();
    failures += test_token_hex_roundtrip();
    failures += test_token_zero();

    failures += test_hs_stats();

    printf("\n");
    if (failures == 0)
        printf("ALL SESSION HS TESTS PASSED\n");
    else
        printf("%d SESSION HS TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
