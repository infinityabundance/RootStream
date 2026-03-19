/*
 * relay_token.h — HMAC-based relay auth token generation and validation
 *
 * Generates 32-byte relay auth tokens using HMAC-SHA256 over a
 * deterministic payload (peer public key + session nonce).  Because
 * libsodium may not be available in all build configs, the module
 * ships a portable fallback HMAC-SHA256 implementation that depends
 * only on a built-in SHA-256 routine.
 *
 * Security note: the token is 256 bits which provides 128-bit
 * collision resistance.  Tokens are single-use; the relay server
 * must invalidate a token once it has been used to pair a session.
 */

#ifndef ROOTSTREAM_RELAY_TOKEN_H
#define ROOTSTREAM_RELAY_TOKEN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RELAY_TOKEN_BYTES 32 /**< Token length in bytes */
#define RELAY_KEY_BYTES 32   /**< HMAC key length in bytes */

/**
 * relay_token_generate — generate a 32-byte auth token
 *
 * Computes HMAC-SHA256( key, peer_pubkey || nonce ) and writes the
 * 32-byte result into @out_token.
 *
 * @param key         32-byte server secret key
 * @param peer_pubkey 32-byte peer public key (e.g. Ed25519 public key)
 * @param nonce       8-byte session nonce
 * @param out_token   Output buffer, must be >= RELAY_TOKEN_BYTES
 */
void relay_token_generate(const uint8_t *key, const uint8_t *peer_pubkey, const uint8_t *nonce,
                          uint8_t *out_token);

/**
 * relay_token_validate — constant-time token comparison
 *
 * @param expected  32-byte reference token
 * @param provided  32-byte token from the client
 * @return          true if tokens match, false otherwise
 */
bool relay_token_validate(const uint8_t *expected, const uint8_t *provided);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_RELAY_TOKEN_H */
