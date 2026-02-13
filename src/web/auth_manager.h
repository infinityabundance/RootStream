/**
 * PHASE 19: Web Dashboard - Authentication Manager
 * 
 * Provides JWT-based authentication and RBAC
 */

#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include "models.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Authentication manager handle
 */
typedef struct auth_manager auth_manager_t;

/**
 * Initialize authentication manager
 */
auth_manager_t *auth_manager_init(void);

/**
 * Add user
 */
int auth_manager_add_user(auth_manager_t *auth,
                         const char *username,
                         const char *password,
                         user_role_t role);

/**
 * Remove user
 */
int auth_manager_remove_user(auth_manager_t *auth,
                            const char *username);

/**
 * Change password
 */
int auth_manager_change_password(auth_manager_t *auth,
                                const char *username,
                                const char *new_password);

/**
 * Authenticate user and generate token
 */
int auth_manager_authenticate(auth_manager_t *auth,
                             const char *username,
                             const char *password,
                             char *token_out,
                             size_t token_size);

/**
 * Verify token and get user info
 */
int auth_manager_verify_token(auth_manager_t *auth,
                              const char *token,
                              char *username_out,
                              size_t username_size,
                              user_role_t *role_out);

/**
 * Check if role can control streaming
 */
bool auth_manager_can_control_streaming(user_role_t role);

/**
 * Check if role can modify settings
 */
bool auth_manager_can_modify_settings(user_role_t role);

/**
 * Check if role can manage users
 */
bool auth_manager_can_manage_users(user_role_t role);

/**
 * Create session
 */
int auth_manager_create_session(auth_manager_t *auth,
                                const char *username);

/**
 * Invalidate session/token
 */
int auth_manager_invalidate_session(auth_manager_t *auth,
                                    const char *token);

/**
 * Cleanup authentication manager
 */
void auth_manager_cleanup(auth_manager_t *auth);

#ifdef __cplusplus
}
#endif

#endif // AUTH_MANAGER_H
