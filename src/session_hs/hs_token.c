/*
 * hs_token.c — Session token generation and comparison
 */

#include "hs_token.h"

#include <string.h>
#include <stdio.h>

/* FNV-1a 32-bit constants */
#define FNV_PRIME   0x01000193U
#define FNV_OFFSET  0x811c9dc5U

int hs_token_from_seed(const uint8_t *seed, size_t seed_sz, hs_token_t *out) {
    if (!seed || !out || seed_sz != HS_TOKEN_SIZE) return -1;

    /* Mix each byte of the seed with FNV-1a to avoid trivial all-zero outputs */
    uint32_t state[4] = {
        FNV_OFFSET ^ 0x1A2B3C4DU,
        FNV_OFFSET ^ 0x5E6F7081U,
        FNV_OFFSET ^ 0x92A3B4C5U,
        FNV_OFFSET ^ 0xD6E7F800U,
    };

    for (int w = 0; w < 4; w++) {
        for (int i = w * 4; i < (w + 1) * 4; i++) {
            state[w] ^= seed[i];
            state[w] *= FNV_PRIME;
        }
    }

    /* Write four 32-bit words into the 16-byte token (little-endian) */
    for (int w = 0; w < 4; w++) {
        out->bytes[w*4+0] = (uint8_t)(state[w]);
        out->bytes[w*4+1] = (uint8_t)(state[w] >> 8);
        out->bytes[w*4+2] = (uint8_t)(state[w] >> 16);
        out->bytes[w*4+3] = (uint8_t)(state[w] >> 24);
    }
    return 0;
}

bool hs_token_equal(const hs_token_t *a, const hs_token_t *b) {
    if (!a || !b) return false;
    /* Constant-time compare: accumulate XOR */
    uint8_t diff = 0;
    for (int i = 0; i < HS_TOKEN_SIZE; i++) diff |= a->bytes[i] ^ b->bytes[i];
    return diff == 0;
}

bool hs_token_zero(const hs_token_t *t) {
    if (!t) return true;
    uint8_t acc = 0;
    for (int i = 0; i < HS_TOKEN_SIZE; i++) acc |= t->bytes[i];
    return acc == 0;
}

int hs_token_to_hex(const hs_token_t *t, char *buf, size_t bufsz) {
    if (!t || !buf || bufsz < (HS_TOKEN_SIZE * 2 + 1)) return -1;
    for (int i = 0; i < HS_TOKEN_SIZE; i++) {
        snprintf(buf + i * 2, 3, "%02x", (unsigned)t->bytes[i]);
    }
    buf[HS_TOKEN_SIZE * 2] = '\0';
    return 0;
}

int hs_token_from_hex(const char *hex, hs_token_t *out) {
    if (!hex || !out) return -1;
    if (strlen(hex) != HS_TOKEN_SIZE * 2) return -1;
    for (int i = 0; i < HS_TOKEN_SIZE; i++) {
        unsigned v;
        if (sscanf(hex + i * 2, "%02x", &v) != 1) return -1;
        out->bytes[i] = (uint8_t)v;
    }
    return 0;
}
