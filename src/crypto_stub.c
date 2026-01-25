/*
 * crypto_stub.c - Stubbed cryptography for NO_CRYPTO builds
 *
 * This is a compile-only fallback for environments without libsodium.
 * Every entry point reports why cryptography is unavailable.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <string.h>

int crypto_init(void) {
    fprintf(stderr, "ERROR: Crypto unavailable (NO_CRYPTO build)\n");
    fprintf(stderr, "FIX: Install libsodium and rebuild without NO_CRYPTO=1\n");
    return -1;
}

int crypto_generate_keypair(keypair_t *kp, const char *hostname) {
    if (!kp || !hostname) {
        fprintf(stderr, "ERROR: crypto_generate_keypair invalid arguments\n");
        return -1;
    }
    fprintf(stderr, "ERROR: crypto_generate_keypair unavailable (NO_CRYPTO build)\n");
    fprintf(stderr, "HOSTNAME: %s\n", hostname);
    return -1;
}

int crypto_load_keypair(keypair_t *kp, const char *config_dir) {
    if (!kp || !config_dir) {
        fprintf(stderr, "ERROR: crypto_load_keypair invalid arguments\n");
        return -1;
    }
    fprintf(stderr, "ERROR: crypto_load_keypair unavailable (NO_CRYPTO build)\n");
    fprintf(stderr, "CONFIG: %s\n", config_dir);
    return -1;
}

int crypto_save_keypair(const keypair_t *kp, const char *config_dir) {
    if (!kp || !config_dir) {
        fprintf(stderr, "ERROR: crypto_save_keypair invalid arguments\n");
        return -1;
    }
    fprintf(stderr, "ERROR: crypto_save_keypair unavailable (NO_CRYPTO build)\n");
    fprintf(stderr, "CONFIG: %s\n", config_dir);
    return -1;
}

int crypto_verify_peer(const uint8_t *public_key, size_t key_len) {
    (void)public_key;
    (void)key_len;
    fprintf(stderr, "ERROR: crypto_verify_peer unavailable (NO_CRYPTO build)\n");
    return -1;
}

int crypto_create_session(crypto_session_t *session,
                          const uint8_t *my_secret,
                          const uint8_t *peer_public) {
    (void)session;
    (void)my_secret;
    (void)peer_public;
    fprintf(stderr, "ERROR: crypto_create_session unavailable (NO_CRYPTO build)\n");
    return -1;
}

int crypto_encrypt_packet(const crypto_session_t *session,
                         const void *plaintext, size_t plain_len,
                         void *ciphertext, size_t *cipher_len,
                         uint64_t nonce) {
    (void)session;
    (void)plaintext;
    (void)plain_len;
    (void)ciphertext;
    (void)cipher_len;
    (void)nonce;
    fprintf(stderr, "ERROR: crypto_encrypt_packet unavailable (NO_CRYPTO build)\n");
    return -1;
}

int crypto_decrypt_packet(const crypto_session_t *session,
                         const void *ciphertext, size_t cipher_len,
                         void *plaintext, size_t *plain_len,
                         uint64_t nonce) {
    (void)session;
    (void)ciphertext;
    (void)cipher_len;
    (void)plaintext;
    (void)plain_len;
    (void)nonce;
    fprintf(stderr, "ERROR: crypto_decrypt_packet unavailable (NO_CRYPTO build)\n");
    return -1;
}
