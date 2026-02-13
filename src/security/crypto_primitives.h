/*
 * crypto_primitives.h - Low-level cryptographic primitives for RootStream
 * 
 * Provides AES-256-GCM, ChaCha20-Poly1305, key derivation, and secure operations
 * Built on libsodium for robust, audited implementations
 */

#ifndef ROOTSTREAM_CRYPTO_PRIMITIVES_H
#define ROOTSTREAM_CRYPTO_PRIMITIVES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Cryptographic constants */
#define CRYPTO_PRIM_KEY_BYTES 32
#define CRYPTO_PRIM_NONCE_BYTES 12
#define CRYPTO_PRIM_TAG_BYTES 16
#define CRYPTO_PRIM_NONCE_XCHACHA_BYTES 24

/*
 * AES-256-GCM encryption (authenticated encryption with associated data)
 * 
 * @param plaintext     Input plaintext data
 * @param plaintext_len Length of plaintext
 * @param key           32-byte encryption key
 * @param nonce         12-byte nonce (must be unique per key)
 * @param aad           Additional authenticated data (can be NULL)
 * @param aad_len       Length of AAD
 * @param ciphertext    Output buffer for ciphertext (same size as plaintext)
 * @param tag           Output buffer for 16-byte authentication tag
 * @return              0 on success, -1 on error
 */
int crypto_prim_aes256gcm_encrypt(
    const uint8_t *plaintext, size_t plaintext_len,
    const uint8_t *key,
    const uint8_t *nonce,
    const uint8_t *aad, size_t aad_len,
    uint8_t *ciphertext,
    uint8_t *tag);

/*
 * AES-256-GCM decryption
 * 
 * @return 0 on success, -1 on authentication failure or error
 */
int crypto_prim_aes256gcm_decrypt(
    const uint8_t *ciphertext, size_t ciphertext_len,
    const uint8_t *key,
    const uint8_t *nonce,
    const uint8_t *aad, size_t aad_len,
    const uint8_t *tag,
    uint8_t *plaintext);

/*
 * ChaCha20-Poly1305 encryption (authenticated encryption)
 * 
 * @return 0 on success, -1 on error
 */
int crypto_prim_chacha20poly1305_encrypt(
    const uint8_t *plaintext, size_t plaintext_len,
    const uint8_t *key,
    const uint8_t *nonce,
    const uint8_t *aad, size_t aad_len,
    uint8_t *ciphertext,
    uint8_t *tag);

/*
 * ChaCha20-Poly1305 decryption
 * 
 * @return 0 on success, -1 on authentication failure or error
 */
int crypto_prim_chacha20poly1305_decrypt(
    const uint8_t *ciphertext, size_t ciphertext_len,
    const uint8_t *key,
    const uint8_t *nonce,
    const uint8_t *aad, size_t aad_len,
    const uint8_t *tag,
    uint8_t *plaintext);

/*
 * Generate cryptographically secure random bytes
 * 
 * @param buffer Output buffer
 * @param size   Number of bytes to generate
 * @return       0 on success, -1 on error
 */
int crypto_prim_random_bytes(uint8_t *buffer, size_t size);

/*
 * HKDF key derivation (HMAC-based Key Derivation Function)
 * 
 * @param input_key_material Input key material
 * @param ikm_len            Length of IKM
 * @param salt               Optional salt (can be NULL)
 * @param salt_len           Length of salt
 * @param info               Optional context info (can be NULL)
 * @param info_len           Length of info
 * @param output_key         Output buffer for derived key
 * @param output_len         Desired output length
 * @return                   0 on success, -1 on error
 */
int crypto_prim_hkdf(
    const uint8_t *input_key_material, size_t ikm_len,
    const uint8_t *salt, size_t salt_len,
    const uint8_t *info, size_t info_len,
    uint8_t *output_key, size_t output_len);

/*
 * Constant-time comparison (prevents timing attacks)
 * 
 * @param a   First buffer
 * @param b   Second buffer
 * @param len Length to compare
 * @return    true if equal, false otherwise
 */
bool crypto_prim_constant_time_compare(
    const uint8_t *a, const uint8_t *b, size_t len);

/*
 * Secure memory wipe (overwrite sensitive data)
 * 
 * @param buffer Memory to wipe
 * @param size   Size to wipe
 */
void crypto_prim_secure_wipe(void *buffer, size_t size);

/*
 * Initialize crypto primitives (must be called before use)
 * 
 * @return 0 on success, -1 on error
 */
int crypto_prim_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CRYPTO_PRIMITIVES_H */
