/*
 * security_manager.h - Main security coordinator for RootStream
 * 
 * Integrates all security components: crypto, key exchange, auth, sessions,
 * attack prevention, and audit logging
 */

#ifndef ROOTSTREAM_SECURITY_MANAGER_H
#define ROOTSTREAM_SECURITY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Security configuration */
typedef struct {
    bool enable_audit_logging;
    bool enforce_session_timeout;
    bool enable_rate_limiting;
    uint32_t session_timeout_sec;
    uint32_t max_requests_per_min;
    const char *audit_log_path;
} security_config_t;

/*
 * Initialize security manager with configuration
 * 
 * @param config  Security configuration (NULL for defaults)
 * @return        0 on success, -1 on error
 */
int security_manager_init(const security_config_t *config);

/*
 * Authenticate user with password
 * 
 * @param username      Username
 * @param password      Password
 * @param session_token Output buffer for session token (at least 65 bytes)
 * @return              0 on success, -1 on error
 */
int security_manager_authenticate(
    const char *username,
    const char *password,
    char *session_token);

/*
 * Validate session token
 * 
 * @param token  Session token to validate
 * @return       true if valid, false otherwise
 */
bool security_manager_validate_session(const char *token);

/*
 * Logout user (invalidate session)
 * 
 * @param session_token  Session token to invalidate
 * @return               0 on success, -1 on error
 */
int security_manager_logout(const char *session_token);

/*
 * Perform secure key exchange with peer
 * 
 * @param peer_public_key  Peer's public key (32 bytes)
 * @param shared_secret    Output shared secret (32 bytes)
 * @return                 0 on success, -1 on error
 */
int security_manager_key_exchange(
    const uint8_t *peer_public_key,
    uint8_t *shared_secret);

/*
 * Encrypt packet data
 * 
 * @param plaintext      Input plaintext
 * @param plaintext_len  Length of plaintext
 * @param key            Encryption key (32 bytes)
 * @param nonce          Nonce (12 bytes)
 * @param ciphertext     Output buffer for ciphertext
 * @param tag            Output buffer for auth tag (16 bytes)
 * @return               0 on success, -1 on error
 */
int security_manager_encrypt(
    const uint8_t *plaintext, size_t plaintext_len,
    const uint8_t *key,
    const uint8_t *nonce,
    uint8_t *ciphertext,
    uint8_t *tag);

/*
 * Decrypt packet data
 * 
 * @param ciphertext     Input ciphertext
 * @param ciphertext_len Length of ciphertext
 * @param key            Decryption key (32 bytes)
 * @param nonce          Nonce (12 bytes)
 * @param tag            Auth tag (16 bytes)
 * @param plaintext      Output buffer for plaintext
 * @return               0 on success, -1 on error
 */
int security_manager_decrypt(
    const uint8_t *ciphertext, size_t ciphertext_len,
    const uint8_t *key,
    const uint8_t *nonce,
    const uint8_t *tag,
    uint8_t *plaintext);

/*
 * Get security statistics
 * 
 * @param stats_json  Output buffer for JSON statistics
 * @param buf_len     Length of buffer
 * @return            0 on success, -1 on error
 */
int security_manager_get_stats(char *stats_json, size_t buf_len);

/*
 * Cleanup security manager
 */
void security_manager_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SECURITY_MANAGER_H */
