/*
 * attack_prevention.h - Protection against common attacks
 */

#ifndef ROOTSTREAM_ATTACK_PREVENTION_H
#define ROOTSTREAM_ATTACK_PREVENTION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize attack prevention
 */
int attack_prevention_init(void);

/*
 * Check if nonce is valid (not already used)
 * 
 * @param nonce     Nonce to check
 * @param nonce_len Length of nonce
 * @return          true if valid (not seen before), false if replay
 */
bool attack_prevention_check_nonce(const uint8_t *nonce, size_t nonce_len);

/*
 * Record failed login attempt
 * 
 * @param username  Username
 * @return          0 on success, -1 on error
 */
int attack_prevention_record_failed_login(const char *username);

/*
 * Check if account is locked due to brute force
 * 
 * @param username  Username to check
 * @return          true if locked, false otherwise
 */
bool attack_prevention_is_account_locked(const char *username);

/*
 * Reset failed login attempts
 * 
 * @param username  Username
 * @return          0 on success, -1 on error
 */
int attack_prevention_reset_failed_attempts(const char *username);

/*
 * Check rate limiting for client
 * 
 * @param client_id  Client identifier
 * @param max_per_min Maximum requests per minute
 * @return           true if rate limited, false if allowed
 */
bool attack_prevention_is_rate_limited(const char *client_id, uint32_t max_per_min);

/*
 * Cleanup attack prevention
 */
void attack_prevention_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_ATTACK_PREVENTION_H */
