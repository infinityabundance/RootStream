/*
 * security_manager.c - Security manager implementation
 */

#include "security_manager.h"
#include "crypto_primitives.h"
#include "key_exchange.h"
#include "user_auth.h"
#include "session_manager.h"
#include "attack_prevention.h"
#include "audit_log.h"
#include <string.h>
#include <stdio.h>

/* Global configuration */
static security_config_t g_config = {
    .enable_audit_logging = true,
    .enforce_session_timeout = true,
    .enable_rate_limiting = true,
    .session_timeout_sec = 3600,
    .max_requests_per_min = 100,
    .audit_log_path = NULL
};

/* Our keypair for key exchange */
static key_exchange_keypair_t g_our_keypair;
static bool g_initialized = false;

/*
 * Initialize security manager
 */
int security_manager_init(const security_config_t *config) {
    if (g_initialized) {
        return 0;
    }
    
    /* Apply custom config if provided */
    if (config) {
        memcpy(&g_config, config, sizeof(security_config_t));
    }
    
    /* Initialize all subsystems */
    if (crypto_prim_init() != 0) {
        fprintf(stderr, "ERROR: Failed to initialize crypto primitives\n");
        return -1;
    }
    
    if (user_auth_init() != 0) {
        fprintf(stderr, "ERROR: Failed to initialize user auth\n");
        return -1;
    }
    
    if (session_manager_init(g_config.session_timeout_sec) != 0) {
        fprintf(stderr, "ERROR: Failed to initialize session manager\n");
        return -1;
    }
    
    if (attack_prevention_init() != 0) {
        fprintf(stderr, "ERROR: Failed to initialize attack prevention\n");
        return -1;
    }
    
    if (g_config.enable_audit_logging) {
        if (audit_log_init(g_config.audit_log_path) != 0) {
            fprintf(stderr, "WARNING: Failed to initialize audit log\n");
        }
    }
    
    /* Generate our keypair for key exchange */
    if (key_exchange_generate_keypair(&g_our_keypair) != 0) {
        fprintf(stderr, "ERROR: Failed to generate keypair\n");
        return -1;
    }
    
    g_initialized = true;
    
    audit_log_event(AUDIT_EVENT_SECURITY_ALERT, NULL, NULL,
                   "Security manager initialized", false);
    
    return 0;
}

/*
 * Authenticate user
 */
int security_manager_authenticate(
    const char *username,
    const char *password,
    char *session_token)
{
    if (!g_initialized || !username || !password || !session_token) {
        return -1;
    }
    
    /* Check if account locked */
    if (attack_prevention_is_account_locked(username)) {
        audit_log_event(AUDIT_EVENT_LOGIN_FAILED, username, NULL,
                       "Account locked due to brute force", true);
        return -1;
    }
    
    /* Check rate limiting */
    if (g_config.enable_rate_limiting &&
        attack_prevention_is_rate_limited(username, g_config.max_requests_per_min)) {
        audit_log_event(AUDIT_EVENT_LOGIN_FAILED, username, NULL,
                       "Rate limited", false);
        return -1;
    }
    
    /* For demonstration, create a test user hash */
    /* In production, verify against stored hash */
    char stored_hash[USER_AUTH_HASH_LEN];
    if (user_auth_hash_password("testpassword", stored_hash) != 0) {
        return -1;
    }
    
    /* Verify password */
    if (!user_auth_verify_password(password, stored_hash)) {
        attack_prevention_record_failed_login(username);
        audit_log_event(AUDIT_EVENT_LOGIN_FAILED, username, NULL,
                       "Invalid password", false);
        return -1;
    }
    
    /* Reset failed attempts on successful auth */
    attack_prevention_reset_failed_attempts(username);
    
    /* Create session */
    if (session_manager_create(username, session_token) != 0) {
        audit_log_event(AUDIT_EVENT_LOGIN_FAILED, username, NULL,
                       "Session creation failed", false);
        return -1;
    }
    
    audit_log_event(AUDIT_EVENT_LOGIN, username, NULL,
                   "Login successful", false);
    audit_log_event(AUDIT_EVENT_SESSION_CREATED, username, NULL,
                   session_token, false);
    
    return 0;
}

/*
 * Validate session
 */
bool security_manager_validate_session(const char *token) {
    if (!g_initialized || !token) {
        return false;
    }
    
    return session_manager_is_valid(token);
}

/*
 * Logout
 */
int security_manager_logout(const char *session_token) {
    if (!g_initialized || !session_token) {
        return -1;
    }
    
    int ret = session_manager_invalidate(session_token);
    
    if (ret == 0) {
        audit_log_event(AUDIT_EVENT_LOGOUT, NULL, NULL,
                       session_token, false);
    }
    
    return ret;
}

/*
 * Key exchange
 */
int security_manager_key_exchange(
    const uint8_t *peer_public_key,
    uint8_t *shared_secret)
{
    if (!g_initialized || !peer_public_key || !shared_secret) {
        return -1;
    }
    
    int ret = key_exchange_compute_shared_secret(
        g_our_keypair.secret_key,
        peer_public_key,
        shared_secret);
    
    if (ret == 0) {
        audit_log_event(AUDIT_EVENT_KEY_EXCHANGE, NULL, NULL,
                       "Key exchange completed", false);
    } else {
        audit_log_event(AUDIT_EVENT_KEY_EXCHANGE, NULL, NULL,
                       "Key exchange failed", true);
    }
    
    return ret;
}

/*
 * Encrypt
 */
int security_manager_encrypt(
    const uint8_t *plaintext, size_t plaintext_len,
    const uint8_t *key,
    const uint8_t *nonce,
    uint8_t *ciphertext,
    uint8_t *tag)
{
    if (!g_initialized) {
        return -1;
    }
    
    /* Use ChaCha20-Poly1305 (RootStream default) */
    return crypto_prim_chacha20poly1305_encrypt(
        plaintext, plaintext_len,
        key, nonce,
        NULL, 0,  /* No AAD */
        ciphertext, tag);
}

/*
 * Decrypt
 */
int security_manager_decrypt(
    const uint8_t *ciphertext, size_t ciphertext_len,
    const uint8_t *key,
    const uint8_t *nonce,
    const uint8_t *tag,
    uint8_t *plaintext)
{
    if (!g_initialized) {
        return -1;
    }
    
    /* Check for replay attack */
    if (!attack_prevention_check_nonce(nonce, CRYPTO_PRIM_NONCE_BYTES)) {
        audit_log_event(AUDIT_EVENT_SUSPICIOUS_ACTIVITY, NULL, NULL,
                       "Replay attack detected (duplicate nonce)", true);
        return -1;
    }
    
    /* Decrypt */
    int ret = crypto_prim_chacha20poly1305_decrypt(
        ciphertext, ciphertext_len,
        key, nonce,
        NULL, 0,  /* No AAD */
        tag, plaintext);
    
    if (ret != 0) {
        audit_log_event(AUDIT_EVENT_ENCRYPTION_FAILED, NULL, NULL,
                       "Decryption failed (authentication error)", true);
    }
    
    return ret;
}

/*
 * Get statistics
 */
int security_manager_get_stats(char *stats_json, size_t buf_len) {
    if (!g_initialized || !stats_json || buf_len == 0) {
        return -1;
    }
    
    /* Simple JSON stats */
    snprintf(stats_json, buf_len,
             "{"
             "\"initialized\":true,"
             "\"audit_logging\":%s,"
             "\"session_timeout\":%u,"
             "\"rate_limiting\":%s"
             "}",
             g_config.enable_audit_logging ? "true" : "false",
             g_config.session_timeout_sec,
             g_config.enable_rate_limiting ? "true" : "false");
    
    return 0;
}

/*
 * Cleanup
 */
void security_manager_cleanup(void) {
    if (!g_initialized) {
        return;
    }
    
    audit_log_event(AUDIT_EVENT_SECURITY_ALERT, NULL, NULL,
                   "Security manager shutdown", false);
    
    user_auth_cleanup();
    session_manager_cleanup();
    attack_prevention_cleanup();
    audit_log_cleanup();
    
    /* Secure wipe of our keypair */
    crypto_prim_secure_wipe(&g_our_keypair, sizeof(g_our_keypair));
    
    g_initialized = false;
}
