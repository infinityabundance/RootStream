/*
 * test_crypto.c - Unit tests for cryptographic functions
 *
 * Tests:
 * - Key generation
 * - Session creation
 * - Encryption/decryption roundtrip
 * - Fingerprint formatting
 * - Peer verification
 */

#include "../../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        printf("  [TEST] %s... ", #name); \
        fflush(stdout); \
        tests_run++; \
        test_##name(); \
        tests_passed++; \
        printf("✓\n"); \
    } \
    static void test_##name(void)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("✗\n"); \
        fprintf(stderr, "    FAILED: %s (line %d)\n", #cond, __LINE__); \
        tests_failed++; \
        tests_passed--; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0)

/* ============================================================================
 * Tests
 * ============================================================================ */

TEST(crypto_init) {
    int result = crypto_init();
    ASSERT_EQ(result, 0);
}

TEST(keypair_generation) {
    keypair_t kp;
    memset(&kp, 0, sizeof(kp));

    int result = crypto_generate_keypair(&kp, "test-host");
    ASSERT_EQ(result, 0);

    /* Check keys are non-zero */
    int zero_count = 0;
    for (int i = 0; i < CRYPTO_PUBLIC_KEY_BYTES; i++) {
        if (kp.public_key[i] == 0) zero_count++;
    }
    ASSERT(zero_count < CRYPTO_PUBLIC_KEY_BYTES);  /* Not all zeros */

    /* Check identity is set */
    ASSERT(strlen(kp.identity) > 0);

    /* Check RootStream code is generated */
    ASSERT(strlen(kp.rootstream_code) > 0);
    ASSERT(strchr(kp.rootstream_code, '@') != NULL);  /* Contains @ separator */
}

TEST(keypair_uniqueness) {
    keypair_t kp1, kp2;

    crypto_generate_keypair(&kp1, "host1");
    crypto_generate_keypair(&kp2, "host2");

    /* Keys should be different */
    ASSERT(memcmp(kp1.public_key, kp2.public_key, CRYPTO_PUBLIC_KEY_BYTES) != 0);
    ASSERT(memcmp(kp1.secret_key, kp2.secret_key, CRYPTO_SECRET_KEY_BYTES) != 0);
}

TEST(session_creation) {
    keypair_t alice, bob;

    crypto_generate_keypair(&alice, "alice");
    crypto_generate_keypair(&bob, "bob");

    /* Create sessions from both sides */
    crypto_session_t alice_session, bob_session;

    int result1 = crypto_create_session(&alice_session, alice.secret_key, bob.public_key);
    int result2 = crypto_create_session(&bob_session, bob.secret_key, alice.public_key);

    ASSERT_EQ(result1, 0);
    ASSERT_EQ(result2, 0);

    /* Both sessions should derive the same shared key */
    ASSERT(memcmp(alice_session.shared_key, bob_session.shared_key,
                  CRYPTO_SHARED_KEY_BYTES) == 0);

    /* Sessions should be marked as authenticated */
    ASSERT(alice_session.authenticated);
    ASSERT(bob_session.authenticated);
}

TEST(encrypt_decrypt_roundtrip) {
    keypair_t alice, bob;
    crypto_generate_keypair(&alice, "alice");
    crypto_generate_keypair(&bob, "bob");

    crypto_session_t alice_session, bob_session;
    crypto_create_session(&alice_session, alice.secret_key, bob.public_key);
    crypto_create_session(&bob_session, bob.secret_key, alice.public_key);

    /* Test message */
    const char *plaintext = "Hello, secure world!";
    size_t plain_len = strlen(plaintext) + 1;

    /* Encrypt */
    uint8_t ciphertext[256];
    size_t cipher_len = 0;
    uint64_t nonce = 12345;

    int enc_result = crypto_encrypt_packet(&alice_session, plaintext, plain_len,
                                           ciphertext, &cipher_len, nonce);
    ASSERT_EQ(enc_result, 0);
    ASSERT(cipher_len > plain_len);  /* Ciphertext includes MAC */

    /* Decrypt */
    uint8_t decrypted[256];
    size_t decrypted_len = 0;

    int dec_result = crypto_decrypt_packet(&bob_session, ciphertext, cipher_len,
                                           decrypted, &decrypted_len, nonce);
    ASSERT_EQ(dec_result, 0);
    ASSERT_EQ(decrypted_len, plain_len);
    ASSERT_STR_EQ((char*)decrypted, plaintext);
}

TEST(decrypt_wrong_nonce_fails) {
    keypair_t alice, bob;
    crypto_generate_keypair(&alice, "alice");
    crypto_generate_keypair(&bob, "bob");

    crypto_session_t alice_session, bob_session;
    crypto_create_session(&alice_session, alice.secret_key, bob.public_key);
    crypto_create_session(&bob_session, bob.secret_key, alice.public_key);

    const char *plaintext = "Secret message";
    size_t plain_len = strlen(plaintext) + 1;

    uint8_t ciphertext[256];
    size_t cipher_len = 0;

    crypto_encrypt_packet(&alice_session, plaintext, plain_len,
                         ciphertext, &cipher_len, 100);

    /* Try to decrypt with wrong nonce */
    uint8_t decrypted[256];
    size_t decrypted_len = 0;

    int result = crypto_decrypt_packet(&bob_session, ciphertext, cipher_len,
                                       decrypted, &decrypted_len, 999);  /* Wrong nonce */
    ASSERT_NE(result, 0);  /* Should fail */
}

TEST(decrypt_tampered_fails) {
    keypair_t alice, bob;
    crypto_generate_keypair(&alice, "alice");
    crypto_generate_keypair(&bob, "bob");

    crypto_session_t alice_session, bob_session;
    crypto_create_session(&alice_session, alice.secret_key, bob.public_key);
    crypto_create_session(&bob_session, bob.secret_key, alice.public_key);

    const char *plaintext = "Tamper test";
    size_t plain_len = strlen(plaintext) + 1;

    uint8_t ciphertext[256];
    size_t cipher_len = 0;
    uint64_t nonce = 42;

    crypto_encrypt_packet(&alice_session, plaintext, plain_len,
                         ciphertext, &cipher_len, nonce);

    /* Tamper with ciphertext */
    ciphertext[5] ^= 0xFF;

    /* Decryption should fail due to MAC mismatch */
    uint8_t decrypted[256];
    size_t decrypted_len = 0;

    int result = crypto_decrypt_packet(&bob_session, ciphertext, cipher_len,
                                       decrypted, &decrypted_len, nonce);
    ASSERT_NE(result, 0);  /* Should fail */
}

TEST(fingerprint_format) {
    keypair_t kp;
    crypto_generate_keypair(&kp, "test");

    char fingerprint[64];
    int result = crypto_format_fingerprint(kp.public_key, CRYPTO_PUBLIC_KEY_BYTES,
                                           fingerprint, sizeof(fingerprint));
    ASSERT_EQ(result, 0);
    ASSERT(strlen(fingerprint) > 0);
    ASSERT(strlen(fingerprint) < 32);  /* Reasonable length */
}

TEST(peer_verification) {
    keypair_t kp;
    crypto_generate_keypair(&kp, "test");

    /* Valid key should pass */
    int result = crypto_verify_peer(kp.public_key, CRYPTO_PUBLIC_KEY_BYTES);
    ASSERT_EQ(result, 0);

    /* Zero key should fail */
    uint8_t zero_key[CRYPTO_PUBLIC_KEY_BYTES] = {0};
    result = crypto_verify_peer(zero_key, CRYPTO_PUBLIC_KEY_BYTES);
    ASSERT_NE(result, 0);
}

TEST(large_message_encryption) {
    keypair_t alice, bob;
    crypto_generate_keypair(&alice, "alice");
    crypto_generate_keypair(&bob, "bob");

    crypto_session_t alice_session, bob_session;
    crypto_create_session(&alice_session, alice.secret_key, bob.public_key);
    crypto_create_session(&bob_session, bob.secret_key, alice.public_key);

    /* Large message (simulating video frame header) */
    uint8_t large_msg[4096];
    for (int i = 0; i < 4096; i++) {
        large_msg[i] = (uint8_t)(i & 0xFF);
    }

    uint8_t ciphertext[5000];
    size_t cipher_len = 0;

    int enc_result = crypto_encrypt_packet(&alice_session, large_msg, 4096,
                                           ciphertext, &cipher_len, 1);
    ASSERT_EQ(enc_result, 0);

    uint8_t decrypted[5000];
    size_t decrypted_len = 0;

    int dec_result = crypto_decrypt_packet(&bob_session, ciphertext, cipher_len,
                                           decrypted, &decrypted_len, 1);
    ASSERT_EQ(dec_result, 0);
    ASSERT_EQ(decrypted_len, 4096);
    ASSERT(memcmp(decrypted, large_msg, 4096) == 0);
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║  RootStream Crypto Unit Tests                  ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\n");

    /* Initialize crypto first */
    run_test_crypto_init();

    /* Run tests */
    run_test_keypair_generation();
    run_test_keypair_uniqueness();
    run_test_session_creation();
    run_test_encrypt_decrypt_roundtrip();
    run_test_decrypt_wrong_nonce_fails();
    run_test_decrypt_tampered_fails();
    run_test_fingerprint_format();
    run_test_peer_verification();
    run_test_large_message_encryption();

    /* Summary */
    printf("\n");
    printf("════════════════════════════════════════════════\n");
    printf("  Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        printf(" (%d failed)", tests_failed);
    }
    printf("\n");
    printf("════════════════════════════════════════════════\n");
    printf("\n");

    return tests_failed > 0 ? 1 : 0;
}
