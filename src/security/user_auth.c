/*
 * user_auth.c - User authentication implementation
 */

#include "user_auth.h"
#include "crypto_primitives.h"
#include <sodium.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* Session storage (simplified - in production use proper database) */
#define MAX_SESSIONS 64
static user_auth_session_t g_sessions[MAX_SESSIONS];
static int g_session_count = 0;

/*
 * Initialize user authentication
 */
int user_auth_init(void) {
    g_session_count = 0;
    memset(g_sessions, 0, sizeof(g_sessions));
    return crypto_prim_init();
}

/*
 * Hash password using Argon2id
 */
int user_auth_hash_password(const char *password, char *hash) {
    if (!password || !hash) {
        return -1;
    }
    
    /* Use libsodium's pwhash (Argon2id) */
    if (crypto_pwhash_str(
            hash,
            password,
            strlen(password),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        fprintf(stderr, "ERROR: Password hashing failed (out of memory?)\n");
        return -1;
    }
    
    return 0;
}

/*
 * Verify password
 */
bool user_auth_verify_password(const char *password, const char *hash) {
    if (!password || !hash) {
        return false;
    }
    
    return crypto_pwhash_str_verify(hash, password, strlen(password)) == 0;
}

/*
 * Generate TOTP secret (base32-encoded)
 */
int user_auth_generate_totp_secret(char *secret, size_t secret_len) {
    if (!secret || secret_len < USER_AUTH_TOTP_SECRET_LEN) {
        return -1;
    }
    
    /* Generate random bytes */
    uint8_t raw_secret[20];
    crypto_prim_random_bytes(raw_secret, sizeof(raw_secret));
    
    /* Base32 encode (simplified - just hex for now) */
    const char hex[] = "0123456789ABCDEF";
    for (size_t i = 0; i < 20 && i * 2 < secret_len - 1; i++) {
        secret[i * 2] = hex[(raw_secret[i] >> 4) & 0xF];
        secret[i * 2 + 1] = hex[raw_secret[i] & 0xF];
    }
    secret[40] = '\0';
    
    crypto_prim_secure_wipe(raw_secret, sizeof(raw_secret));
    return 0;
}

/*
 * Verify TOTP code (Time-based One-Time Password)
 */
bool user_auth_verify_totp(const char *secret, const char *code) {
    if (!secret || !code) {
        return false;
    }
    
    /* Simplified TOTP implementation */
    /* In production, use proper TOTP library (RFC 6238) */
    
    /* Get current time in 30-second intervals */
    uint64_t time_step = (uint64_t)time(NULL) / 30;
    
    /* For demonstration, accept any 6-digit code */
    /* Real implementation would compute HMAC-SHA1 of time_step with secret */
    if (strlen(code) == 6) {
        for (int i = 0; i < 6; i++) {
            if (code[i] < '0' || code[i] > '9') {
                return false;
            }
        }
        return true;  /* Accept valid format for now */
    }
    
    return false;
}

/*
 * Create session
 */
int user_auth_create_session(const char *username, user_auth_session_t *session) {
    if (!username || !session || g_session_count >= MAX_SESSIONS) {
        return -1;
    }
    
    /* Generate random session token */
    uint8_t token_bytes[32];
    crypto_prim_random_bytes(token_bytes, sizeof(token_bytes));
    
    /* Convert to hex string */
    const char hex[] = "0123456789abcdef";
    for (int i = 0; i < 32; i++) {
        session->session_token[i * 2] = hex[(token_bytes[i] >> 4) & 0xF];
        session->session_token[i * 2 + 1] = hex[token_bytes[i] & 0xF];
    }
    session->session_token[64] = '\0';
    
    /* Set session info */
    strncpy(session->username, username, USER_AUTH_MAX_USERNAME - 1);
    session->username[USER_AUTH_MAX_USERNAME - 1] = '\0';
    
    /* Expiration: 1 hour from now */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    session->expiration_time_us = (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
    session->expiration_time_us += 3600 * 1000000;  /* +1 hour */
    
    session->mfa_verified = false;
    
    /* Store session */
    memcpy(&g_sessions[g_session_count], session, sizeof(user_auth_session_t));
    g_session_count++;
    
    return 0;
}

/*
 * Validate session
 */
bool user_auth_validate_session(const char *token) {
    if (!token) {
        return false;
    }
    
    /* Get current time */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t now_us = (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
    
    /* Check all sessions */
    for (int i = 0; i < g_session_count; i++) {
        if (strcmp(g_sessions[i].session_token, token) == 0) {
            /* Check expiration */
            if (now_us < g_sessions[i].expiration_time_us) {
                return true;
            }
        }
    }
    
    return false;
}

/*
 * Cleanup
 */
void user_auth_cleanup(void) {
    crypto_prim_secure_wipe(g_sessions, sizeof(g_sessions));
    g_session_count = 0;
}
