/*
 * hs_token.h — 128-bit session token generation and comparison
 *
 * Tokens are generated from a caller-supplied entropy source (a
 * 16-byte seed) so they can be fully deterministic in tests without
 * requiring /dev/urandom.  In production, callers pass random bytes.
 *
 * Wire representation: 16 raw bytes (big-endian UUID-style display).
 *
 * Thread-safety: stateless functions — thread-safe.
 */

#ifndef ROOTSTREAM_HS_TOKEN_H
#define ROOTSTREAM_HS_TOKEN_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HS_TOKEN_SIZE  16  /**< Token length in bytes */

/** 128-bit session token */
typedef struct {
    uint8_t bytes[HS_TOKEN_SIZE];
} hs_token_t;

/**
 * hs_token_from_seed — derive token from 16-byte seed
 *
 * Applies a simple FNV-1a mix so that known seeds produce known
 * (non-trivially all-zero) tokens even in tests.
 *
 * @param seed     16-byte entropy seed
 * @param seed_sz  Seed size (must be HS_TOKEN_SIZE)
 * @param out      Output token
 * @return         0 on success, -1 on error
 */
int hs_token_from_seed(const uint8_t *seed, size_t seed_sz, hs_token_t *out);

/**
 * hs_token_equal — constant-time 128-bit comparison
 *
 * @param a  First token
 * @param b  Second token
 * @return   true if equal
 */
bool hs_token_equal(const hs_token_t *a, const hs_token_t *b);

/**
 * hs_token_zero — return true if all bytes are zero
 *
 * @param t  Token
 * @return   true if zero
 */
bool hs_token_zero(const hs_token_t *t);

/**
 * hs_token_to_hex — render token as 32-character NUL-terminated hex string
 *
 * @param t      Token
 * @param buf    Output buffer (>= 33 bytes)
 * @param bufsz  Buffer size
 * @return       0 on success, -1 if buffer too small
 */
int hs_token_to_hex(const hs_token_t *t, char *buf, size_t bufsz);

/**
 * hs_token_from_hex — parse token from 32-character hex string
 *
 * @param hex  Hex string (exactly 32 chars, NUL-terminated)
 * @param out  Output token
 * @return     0 on success, -1 on invalid input
 */
int hs_token_from_hex(const char *hex, hs_token_t *out);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_HS_TOKEN_H */
