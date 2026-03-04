/*
 * test_relay.c — Unit tests for PHASE-40 Relay / TURN Infrastructure
 *
 * Tests relay_protocol (encode/decode), relay_session (lifecycle),
 * relay_client (state machine), and relay_token (HMAC generate/validate).
 * No real network connections required.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/relay/relay_protocol.h"
#include "../../src/relay/relay_session.h"
#include "../../src/relay/relay_client.h"
#include "../../src/relay/relay_token.h"

/* ── Test helpers ────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── relay_protocol tests ────────────────────────────────────────── */

static int test_header_roundtrip(void) {
    printf("\n=== test_header_roundtrip ===\n");

    relay_header_t orig = {
        .type        = RELAY_MSG_DATA,
        .session_id  = 0xDEADBEEF,
        .payload_len = 128,
    };
    uint8_t buf[RELAY_HDR_SIZE];
    int rc = relay_encode_header(&orig, buf);
    TEST_ASSERT(rc == RELAY_HDR_SIZE, "encode returns HDR_SIZE");

    relay_header_t decoded;
    rc = relay_decode_header(buf, &decoded);
    TEST_ASSERT(rc == 0, "decode succeeds");
    TEST_ASSERT(decoded.type        == RELAY_MSG_DATA,  "type preserved");
    TEST_ASSERT(decoded.session_id  == 0xDEADBEEF,      "session_id preserved");
    TEST_ASSERT(decoded.payload_len == 128,              "payload_len preserved");

    TEST_PASS("relay header encode/decode round-trip");
    return 0;
}

static int test_header_bad_magic(void) {
    printf("\n=== test_header_bad_magic ===\n");

    uint8_t buf[RELAY_HDR_SIZE] = {0xFF, 0xFF, RELAY_VERSION, 0x05};
    relay_header_t hdr;
    int rc = relay_decode_header(buf, &hdr);
    TEST_ASSERT(rc == -1, "bad magic returns -1");

    TEST_PASS("relay header rejects bad magic");
    return 0;
}

static int test_hello_roundtrip(void) {
    printf("\n=== test_hello_roundtrip ===\n");

    uint8_t token[RELAY_TOKEN_LEN];
    memset(token, 0xAB, sizeof(token));

    uint8_t payload[36];
    int n = relay_build_hello(token, true, payload);
    TEST_ASSERT(n == 36, "hello payload is 36 bytes");

    uint8_t out_token[RELAY_TOKEN_LEN];
    bool is_host;
    int rc = relay_parse_hello(payload, 36, out_token, &is_host);
    TEST_ASSERT(rc == 0, "parse succeeds");
    TEST_ASSERT(is_host, "host flag preserved");
    TEST_ASSERT(memcmp(token, out_token, RELAY_TOKEN_LEN) == 0,
                "token preserved in HELLO");

    TEST_PASS("relay HELLO build/parse round-trip");
    return 0;
}

/* ── relay_session tests ─────────────────────────────────────────── */

static int test_session_manager_create(void) {
    printf("\n=== test_session_manager_create ===\n");

    relay_session_manager_t *m = relay_session_manager_create();
    TEST_ASSERT(m != NULL, "manager created");
    TEST_ASSERT(relay_session_count(m) == 0, "initial count 0");
    relay_session_manager_destroy(m);
    relay_session_manager_destroy(NULL); /* must not crash */
    TEST_PASS("relay session manager create/destroy");
    return 0;
}

static int test_session_open_close(void) {
    printf("\n=== test_session_open_close ===\n");

    relay_session_manager_t *m = relay_session_manager_create();
    TEST_ASSERT(m != NULL, "manager created");

    uint8_t token[RELAY_TOKEN_LEN];
    memset(token, 0x11, sizeof(token));

    relay_session_id_t id = 0;
    int rc = relay_session_open(m, token, 10, &id);
    TEST_ASSERT(rc == 0,   "session opened");
    TEST_ASSERT(id >= 1,   "id >= 1");
    TEST_ASSERT(relay_session_count(m) == 1, "count 1");

    relay_session_entry_t entry;
    rc = relay_session_get(m, id, &entry);
    TEST_ASSERT(rc == 0, "get session succeeds");
    TEST_ASSERT(entry.state == RELAY_STATE_WAITING, "state WAITING");
    TEST_ASSERT(entry.host_fd == 10, "host_fd set");
    TEST_ASSERT(entry.viewer_fd == -1, "viewer_fd -1");

    rc = relay_session_close(m, id);
    TEST_ASSERT(rc == 0, "close succeeds");
    TEST_ASSERT(relay_session_count(m) == 0, "count 0 after close");

    relay_session_manager_destroy(m);
    TEST_PASS("relay session open/close");
    return 0;
}

static int test_session_pair(void) {
    printf("\n=== test_session_pair ===\n");

    relay_session_manager_t *m = relay_session_manager_create();
    uint8_t token[RELAY_TOKEN_LEN];
    memset(token, 0x22, sizeof(token));

    relay_session_id_t host_id = 0;
    relay_session_open(m, token, 5, &host_id);

    relay_session_id_t viewer_id = 0;
    int rc = relay_session_pair(m, token, 6, &viewer_id);
    TEST_ASSERT(rc == 0, "pair succeeds");
    TEST_ASSERT(viewer_id == host_id, "same session ID");

    relay_session_entry_t entry;
    relay_session_get(m, host_id, &entry);
    TEST_ASSERT(entry.state == RELAY_STATE_PAIRED, "state PAIRED");
    TEST_ASSERT(entry.viewer_fd == 6, "viewer_fd set");

    relay_session_manager_destroy(m);
    TEST_PASS("relay session pairing");
    return 0;
}

static int test_session_pair_wrong_token(void) {
    printf("\n=== test_session_pair_wrong_token ===\n");

    relay_session_manager_t *m = relay_session_manager_create();

    uint8_t token_a[RELAY_TOKEN_LEN], token_b[RELAY_TOKEN_LEN];
    memset(token_a, 0xAA, sizeof(token_a));
    memset(token_b, 0xBB, sizeof(token_b));

    relay_session_id_t id = 0;
    relay_session_open(m, token_a, 5, &id);

    relay_session_id_t vid = 0;
    int rc = relay_session_pair(m, token_b, 6, &vid);
    TEST_ASSERT(rc == -1, "wrong token cannot pair");

    relay_session_manager_destroy(m);
    TEST_PASS("relay session rejects wrong token");
    return 0;
}

static int test_session_bytes(void) {
    printf("\n=== test_session_bytes ===\n");

    relay_session_manager_t *m = relay_session_manager_create();
    uint8_t token[RELAY_TOKEN_LEN];
    memset(token, 0x33, sizeof(token));
    relay_session_id_t id = 0;
    relay_session_open(m, token, 5, &id);

    relay_session_add_bytes(m, id, 1000);
    relay_session_add_bytes(m, id, 500);

    relay_session_entry_t entry;
    relay_session_get(m, id, &entry);
    TEST_ASSERT(entry.bytes_relayed == 1500, "bytes_relayed accumulated");

    relay_session_manager_destroy(m);
    TEST_PASS("relay session bytes counter");
    return 0;
}

/* ── relay_client tests ──────────────────────────────────────────── */

/* Simple write-to-buffer I/O mock */
typedef struct {
    uint8_t  buf[4096];
    size_t   len;
} mock_io_t;

static int mock_send(const uint8_t *data, size_t len, void *ud) {
    mock_io_t *m = (mock_io_t *)ud;
    if (m->len + len > sizeof(m->buf)) return -1;
    memcpy(m->buf + m->len, data, len);
    m->len += len;
    return (int)len;
}

static int test_relay_client_connect(void) {
    printf("\n=== test_relay_client_connect ===\n");

    mock_io_t io_buf = {0};
    relay_io_t io = { .send_fn = mock_send, .user_data = &io_buf };
    uint8_t token[RELAY_TOKEN_LEN];
    memset(token, 0x55, sizeof(token));

    relay_client_t *c = relay_client_create(&io, token, true);
    TEST_ASSERT(c != NULL, "client created");
    TEST_ASSERT(relay_client_get_state(c) == RELAY_CLIENT_DISCONNECTED,
                "initial state DISCONNECTED");

    int rc = relay_client_connect(c);
    TEST_ASSERT(rc == 0, "connect returns 0");
    TEST_ASSERT(relay_client_get_state(c) == RELAY_CLIENT_HELLO_SENT,
                "state HELLO_SENT after connect");
    TEST_ASSERT(io_buf.len == (size_t)(RELAY_HDR_SIZE + 36),
                "HELLO message sent");

    relay_client_destroy(c);
    TEST_PASS("relay client connect sends HELLO");
    return 0;
}

static int test_relay_client_hello_ack(void) {
    printf("\n=== test_relay_client_hello_ack ===\n");

    mock_io_t io_buf = {0};
    relay_io_t io = { .send_fn = mock_send, .user_data = &io_buf };
    uint8_t token[RELAY_TOKEN_LEN];
    memset(token, 0x66, sizeof(token));

    relay_client_t *c = relay_client_create(&io, token, false);
    relay_client_connect(c);

    /* Simulate HELLO_ACK from server */
    uint8_t ack[RELAY_HDR_SIZE];
    relay_header_t ack_hdr = {
        .type        = RELAY_MSG_HELLO_ACK,
        .session_id  = 42,
        .payload_len = 0,
    };
    relay_encode_header(&ack_hdr, ack);

    int rc = relay_client_receive(c, ack, sizeof(ack), NULL, NULL);
    TEST_ASSERT(rc == 0, "receive returns 0");
    TEST_ASSERT(relay_client_get_state(c) == RELAY_CLIENT_READY,
                "state READY after ACK");
    TEST_ASSERT(relay_client_get_session_id(c) == 42,
                "session ID from ACK preserved");

    relay_client_destroy(c);
    TEST_PASS("relay client transitions to READY on HELLO_ACK");
    return 0;
}

static int test_relay_client_ping_pong(void) {
    printf("\n=== test_relay_client_ping_pong ===\n");

    mock_io_t io_buf = {0};
    relay_io_t io = { .send_fn = mock_send, .user_data = &io_buf };
    uint8_t token[RELAY_TOKEN_LEN];
    memset(token, 0x77, sizeof(token));

    relay_client_t *c = relay_client_create(&io, token, true);
    relay_client_connect(c);

    /* Ack to get into READY state */
    uint8_t ack[RELAY_HDR_SIZE];
    relay_header_t ah = { RELAY_MSG_HELLO_ACK, 7, 0 };
    relay_encode_header(&ah, ack);
    relay_client_receive(c, ack, sizeof(ack), NULL, NULL);
    size_t len_before = io_buf.len;

    /* Send PING */
    uint8_t ping[RELAY_HDR_SIZE];
    relay_header_t ph = { RELAY_MSG_PING, 7, 0 };
    relay_encode_header(&ph, ping);
    relay_client_receive(c, ping, sizeof(ping), NULL, NULL);

    /* Client should have sent a PONG */
    TEST_ASSERT(io_buf.len > len_before, "PONG sent in response to PING");

    relay_client_destroy(c);
    TEST_PASS("relay client auto-responds to PING with PONG");
    return 0;
}

/* ── relay_token tests ───────────────────────────────────────────── */

static int test_token_generate_deterministic(void) {
    printf("\n=== test_token_generate_deterministic ===\n");

    uint8_t key[RELAY_KEY_BYTES];
    uint8_t pubkey[32];
    uint8_t nonce[8];
    memset(key,    0x01, sizeof(key));
    memset(pubkey, 0x02, sizeof(pubkey));
    memset(nonce,  0x03, sizeof(nonce));

    uint8_t token1[RELAY_TOKEN_BYTES];
    uint8_t token2[RELAY_TOKEN_BYTES];
    relay_token_generate(key, pubkey, nonce, token1);
    relay_token_generate(key, pubkey, nonce, token2);

    TEST_ASSERT(memcmp(token1, token2, RELAY_TOKEN_BYTES) == 0,
                "same inputs produce same token");
    TEST_PASS("token generation is deterministic");
    return 0;
}

static int test_token_different_key(void) {
    printf("\n=== test_token_different_key ===\n");

    uint8_t key_a[RELAY_KEY_BYTES], key_b[RELAY_KEY_BYTES];
    uint8_t pubkey[32], nonce[8];
    memset(key_a,  0xAA, sizeof(key_a));
    memset(key_b,  0xBB, sizeof(key_b));
    memset(pubkey, 0x02, sizeof(pubkey));
    memset(nonce,  0x03, sizeof(nonce));

    uint8_t token_a[RELAY_TOKEN_BYTES], token_b[RELAY_TOKEN_BYTES];
    relay_token_generate(key_a, pubkey, nonce, token_a);
    relay_token_generate(key_b, pubkey, nonce, token_b);

    TEST_ASSERT(memcmp(token_a, token_b, RELAY_TOKEN_BYTES) != 0,
                "different keys produce different tokens");
    TEST_PASS("token differs with different key");
    return 0;
}

static int test_token_validate(void) {
    printf("\n=== test_token_validate ===\n");

    uint8_t key[RELAY_KEY_BYTES], pubkey[32], nonce[8];
    memset(key, 0x55, sizeof(key));
    memset(pubkey, 0x66, sizeof(pubkey));
    memset(nonce, 0x77, sizeof(nonce));

    uint8_t token[RELAY_TOKEN_BYTES];
    relay_token_generate(key, pubkey, nonce, token);

    TEST_ASSERT(relay_token_validate(token, token), "same token validates");

    uint8_t bad[RELAY_TOKEN_BYTES];
    memcpy(bad, token, RELAY_TOKEN_BYTES);
    bad[0] ^= 0xFF;
    TEST_ASSERT(!relay_token_validate(token, bad),
                "tampered token fails validation");

    TEST_ASSERT(!relay_token_validate(NULL, token), "NULL expected safe");
    TEST_ASSERT(!relay_token_validate(token, NULL), "NULL provided safe");

    TEST_PASS("relay token validate");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_header_roundtrip();
    failures += test_header_bad_magic();
    failures += test_hello_roundtrip();

    failures += test_session_manager_create();
    failures += test_session_open_close();
    failures += test_session_pair();
    failures += test_session_pair_wrong_token();
    failures += test_session_bytes();

    failures += test_relay_client_connect();
    failures += test_relay_client_hello_ack();
    failures += test_relay_client_ping_pong();

    failures += test_token_generate_deterministic();
    failures += test_token_different_key();
    failures += test_token_validate();

    printf("\n");
    if (failures == 0)
        printf("ALL RELAY TESTS PASSED\n");
    else
        printf("%d RELAY TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
