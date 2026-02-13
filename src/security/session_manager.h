/*
 * session_manager.h - Secure session management with perfect forward secrecy
 */

#ifndef ROOTSTREAM_SESSION_MANAGER_H
#define ROOTSTREAM_SESSION_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SESSION_ID_LEN 65  /* 64 hex + null */

/*
 * Initialize session manager
 * 
 * @param timeout_sec  Session timeout in seconds (default: 3600)
 * @return             0 on success, -1 on error
 */
int session_manager_init(uint32_t timeout_sec);

/*
 * Create new session
 * 
 * @param username    Username for session
 * @param session_id  Output buffer for session ID (at least SESSION_ID_LEN)
 * @return            0 on success, -1 on error
 */
int session_manager_create(const char *username, char *session_id);

/*
 * Validate session
 * 
 * @param session_id  Session ID to validate
 * @return            true if valid, false otherwise
 */
bool session_manager_is_valid(const char *session_id);

/*
 * Refresh session (extend timeout)
 * 
 * @param session_id  Session ID to refresh
 * @return            0 on success, -1 on error
 */
int session_manager_refresh(const char *session_id);

/*
 * Invalidate session (logout)
 * 
 * @param session_id  Session ID to invalidate
 * @return            0 on success, -1 on error
 */
int session_manager_invalidate(const char *session_id);

/*
 * Cleanup expired sessions
 * 
 * @return  Number of sessions cleaned up
 */
int session_manager_cleanup_expired(void);

/*
 * Cleanup session manager
 */
void session_manager_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SESSION_MANAGER_H */
