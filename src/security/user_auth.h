/*
 * user_auth.h - User authentication with Argon2 and TOTP
 */

#ifndef ROOTSTREAM_USER_AUTH_H
#define ROOTSTREAM_USER_AUTH_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USER_AUTH_MAX_USERNAME 64
#define USER_AUTH_HASH_LEN 128
#define USER_AUTH_TOTP_SECRET_LEN 32
#define USER_AUTH_TOTP_CODE_LEN 7

/* Session structure */
typedef struct {
    char session_token[65];  /* 64 hex chars + null */
    char username[USER_AUTH_MAX_USERNAME];
    uint64_t expiration_time_us;
    bool mfa_verified;
} user_auth_session_t;

/*
 * Initialize user authentication system
 */
int user_auth_init(void);

/*
 * Hash password using Argon2id
 * 
 * @param password  Input password
 * @param hash      Output hash buffer (at least USER_AUTH_HASH_LEN bytes)
 * @return          0 on success, -1 on error
 */
int user_auth_hash_password(const char *password, char *hash);

/*
 * Verify password against hash
 * 
 * @param password  Input password to verify
 * @param hash      Stored hash
 * @return          true if matches, false otherwise
 */
bool user_auth_verify_password(const char *password, const char *hash);

/*
 * Generate TOTP secret
 * 
 * @param secret    Output buffer for base32-encoded secret
 * @param secret_len Length of buffer
 * @return          0 on success, -1 on error
 */
int user_auth_generate_totp_secret(char *secret, size_t secret_len);

/*
 * Verify TOTP code
 * 
 * @param secret    Base32-encoded TOTP secret
 * @param code      6-digit TOTP code
 * @return          true if valid, false otherwise
 */
bool user_auth_verify_totp(const char *secret, const char *code);

/*
 * Create authentication session
 * 
 * @param username  Username
 * @param session   Output session structure
 * @return          0 on success, -1 on error
 */
int user_auth_create_session(const char *username, user_auth_session_t *session);

/*
 * Validate session token
 * 
 * @param token     Session token
 * @return          true if valid, false otherwise
 */
bool user_auth_validate_session(const char *token);

/*
 * Cleanup user authentication system
 */
void user_auth_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_USER_AUTH_H */
