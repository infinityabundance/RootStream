/*
 * test_security.c - Unit tests for Phase 21 security modules
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../../src/security/crypto_primitives.h"
#include "../../src/security/key_exchange.h"
#include "../../src/security/user_auth.h"
#include "../../src/security/session_manager.h"
#include "../../src/security/attack_prevention.h"
#include "../../src/security/audit_log.h"
#include "../../src/security/security_manager.h"

/* Test counter */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    printf("  Testing %s... ", name); \
    fflush(stdout);

#define PASS() \
    printf("PASS\n"); \
    tests_passed++;

#define FAIL(msg) \
    printf("FAIL: %s\n", msg); \
    tests_failed++;

/* Test crypto primitives */
void test_crypto_primitives(void) {
    printf("\n=== Crypto Primitives Tests ===\n");
    
    /* Test initialization */
    TEST("crypto_prim_init");
    if (crypto_prim_init() == 0) {
        PASS();
    } else {
        FAIL("Initialization failed");
    }
    
    /* Test random bytes */
    TEST("crypto_prim_random_bytes");
    uint8_t random1[32], random2[32];
    if (crypto_prim_random_bytes(random1, 32) == 0 &&
        crypto_prim_random_bytes(random2, 32) == 0 &&
        memcmp(random1, random2, 32) != 0) {
        PASS();
    } else {
        FAIL("Random generation failed or not random");
    }
    
    /* Test ChaCha20-Poly1305 encryption */
    TEST("ChaCha20-Poly1305 encrypt/decrypt");
    uint8_t key[32], nonce[12];
    crypto_prim_random_bytes(key, 32);
    crypto_prim_random_bytes(nonce, 12);
    
    const char *plaintext = "Hello, RootStream!";
    size_t plaintext_len = strlen(plaintext);
    uint8_t ciphertext[256], tag[16];
    uint8_t decrypted[256];
    
    if (crypto_prim_chacha20poly1305_encrypt(
            (const uint8_t *)plaintext, plaintext_len,
            key, nonce, NULL, 0, ciphertext, tag) == 0 &&
        crypto_prim_chacha20poly1305_decrypt(
            ciphertext, plaintext_len,
            key, nonce, NULL, 0, tag, decrypted) == 0 &&
        memcmp(plaintext, decrypted, plaintext_len) == 0) {
        PASS();
    } else {
        FAIL("ChaCha20-Poly1305 failed");
    }
    
    /* Test constant-time compare */
    TEST("constant_time_compare");
    uint8_t a[16], b[16];
    memset(a, 0x42, 16);
    memset(b, 0x42, 16);
    if (crypto_prim_constant_time_compare(a, b, 16) &&
        !crypto_prim_constant_time_compare(a, random1, 16)) {
        PASS();
    } else {
        FAIL("Constant-time compare failed");
    }
}

/* Test key exchange */
void test_key_exchange(void) {
    printf("\n=== Key Exchange Tests ===\n");
    
    /* Test keypair generation */
    TEST("key_exchange_generate_keypair");
    key_exchange_keypair_t kp1, kp2;
    if (key_exchange_generate_keypair(&kp1) == 0 &&
        key_exchange_generate_keypair(&kp2) == 0) {
        PASS();
    } else {
        FAIL("Keypair generation failed");
    }
    
    /* Test ECDH */
    TEST("key_exchange_compute_shared_secret");
    uint8_t secret1[32], secret2[32];
    if (key_exchange_compute_shared_secret(kp1.secret_key, kp2.public_key, secret1) == 0 &&
        key_exchange_compute_shared_secret(kp2.secret_key, kp1.public_key, secret2) == 0 &&
        memcmp(secret1, secret2, 32) == 0) {
        PASS();
    } else {
        FAIL("ECDH failed or secrets don't match");
    }
    
    /* Test session key derivation */
    TEST("key_exchange_derive_session_keys");
    uint8_t c2s_key[32], s2c_key[32], c_nonce[12], s_nonce[12];
    if (key_exchange_derive_session_keys(secret1, c2s_key, s2c_key, c_nonce, s_nonce) == 0) {
        PASS();
    } else {
        FAIL("Session key derivation failed");
    }
}

/* Test user authentication */
void test_user_auth(void) {
    printf("\n=== User Authentication Tests ===\n");
    
    /* Test initialization */
    TEST("user_auth_init");
    if (user_auth_init() == 0) {
        PASS();
    } else {
        FAIL("Initialization failed");
    }
    
    /* Test password hashing */
    TEST("user_auth_hash_password");
    char hash[128];
    if (user_auth_hash_password("testpassword123", hash) == 0) {
        PASS();
    } else {
        FAIL("Password hashing failed");
    }
    
    /* Test password verification */
    TEST("user_auth_verify_password");
    if (user_auth_verify_password("testpassword123", hash) &&
        !user_auth_verify_password("wrongpassword", hash)) {
        PASS();
    } else {
        FAIL("Password verification failed");
    }
    
    /* Test session creation */
    TEST("user_auth_create_session");
    user_auth_session_t session;
    if (user_auth_create_session("testuser", &session) == 0 &&
        strlen(session.session_token) == 64) {
        PASS();
    } else {
        FAIL("Session creation failed");
    }
    
    /* Test session validation */
    TEST("user_auth_validate_session");
    if (user_auth_validate_session(session.session_token)) {
        PASS();
    } else {
        FAIL("Session validation failed");
    }
}

/* Test session manager */
void test_session_manager(void) {
    printf("\n=== Session Manager Tests ===\n");
    
    /* Test initialization */
    TEST("session_manager_init");
    if (session_manager_init(3600) == 0) {
        PASS();
    } else {
        FAIL("Initialization failed");
    }
    
    /* Test session creation */
    TEST("session_manager_create");
    char session_id[65];
    if (session_manager_create("testuser", session_id) == 0) {
        PASS();
    } else {
        FAIL("Session creation failed");
    }
    
    /* Test session validation */
    TEST("session_manager_is_valid");
    if (session_manager_is_valid(session_id)) {
        PASS();
    } else {
        FAIL("Session validation failed");
    }
    
    /* Test session invalidation */
    TEST("session_manager_invalidate");
    if (session_manager_invalidate(session_id) == 0 &&
        !session_manager_is_valid(session_id)) {
        PASS();
    } else {
        FAIL("Session invalidation failed");
    }
}

/* Test attack prevention */
void test_attack_prevention(void) {
    printf("\n=== Attack Prevention Tests ===\n");
    
    /* Test initialization */
    TEST("attack_prevention_init");
    if (attack_prevention_init() == 0) {
        PASS();
    } else {
        FAIL("Initialization failed");
    }
    
    /* Test nonce checking (replay prevention) */
    TEST("attack_prevention_check_nonce");
    uint8_t nonce1[32], nonce2[32];
    crypto_prim_random_bytes(nonce1, 32);
    crypto_prim_random_bytes(nonce2, 32);
    if (attack_prevention_check_nonce(nonce1, 32) &&
        !attack_prevention_check_nonce(nonce1, 32) &&
        attack_prevention_check_nonce(nonce2, 32)) {
        PASS();
    } else {
        FAIL("Nonce checking failed");
    }
    
    /* Test brute force protection */
    TEST("attack_prevention_record_failed_login");
    const char *username = "testuser";
    for (int i = 0; i < 5; i++) {
        attack_prevention_record_failed_login(username);
    }
    if (attack_prevention_is_account_locked(username)) {
        PASS();
    } else {
        FAIL("Account not locked after 5 failed attempts");
    }
    
    /* Test reset */
    TEST("attack_prevention_reset_failed_attempts");
    attack_prevention_reset_failed_attempts(username);
    if (!attack_prevention_is_account_locked(username)) {
        PASS();
    } else {
        FAIL("Account still locked after reset");
    }
}

/* Test security manager */
void test_security_manager(void) {
    printf("\n=== Security Manager Tests ===\n");
    
    /* Test initialization */
    TEST("security_manager_init");
    if (security_manager_init(NULL) == 0) {
        PASS();
    } else {
        FAIL("Initialization failed");
    }
    
    /* Test encryption/decryption */
    TEST("security_manager_encrypt/decrypt");
    uint8_t key[32], nonce[12];
    crypto_prim_random_bytes(key, 32);
    crypto_prim_random_bytes(nonce, 12);
    
    const char *msg = "Test message";
    size_t msg_len = strlen(msg);
    uint8_t ciphertext[256], tag[16], plaintext[256];
    
    if (security_manager_encrypt((const uint8_t *)msg, msg_len, key, nonce,
                                ciphertext, tag) == 0 &&
        security_manager_decrypt(ciphertext, msg_len, key, nonce, tag,
                                plaintext) == 0 &&
        memcmp(msg, plaintext, msg_len) == 0) {
        PASS();
    } else {
        FAIL("Encryption/decryption failed");
    }
    
    /* Test statistics */
    TEST("security_manager_get_stats");
    char stats[1024];
    if (security_manager_get_stats(stats, sizeof(stats)) == 0 &&
        strstr(stats, "initialized") != NULL) {
        PASS();
    } else {
        FAIL("Get stats failed");
    }
}

int main(void) {
    printf("RootStream Phase 21 Security Tests\n");
    printf("===================================\n");
    
    test_crypto_primitives();
    test_key_exchange();
    test_user_auth();
    test_session_manager();
    test_attack_prevention();
    test_security_manager();
    
    printf("\n===================================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("===================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
