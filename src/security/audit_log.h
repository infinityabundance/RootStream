/*
 * audit_log.h - Security audit logging
 */

#ifndef ROOTSTREAM_AUDIT_LOG_H
#define ROOTSTREAM_AUDIT_LOG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Event types */
typedef enum {
    AUDIT_EVENT_LOGIN,
    AUDIT_EVENT_LOGOUT,
    AUDIT_EVENT_LOGIN_FAILED,
    AUDIT_EVENT_SESSION_CREATED,
    AUDIT_EVENT_SESSION_EXPIRED,
    AUDIT_EVENT_KEY_EXCHANGE,
    AUDIT_EVENT_ENCRYPTION_FAILED,
    AUDIT_EVENT_SUSPICIOUS_ACTIVITY,
    AUDIT_EVENT_SECURITY_ALERT
} audit_event_type_t;

/*
 * Initialize audit logging
 * 
 * @param log_file  Path to log file (NULL for stderr)
 * @return          0 on success, -1 on error
 */
int audit_log_init(const char *log_file);

/*
 * Log security event
 * 
 * @param type      Event type
 * @param username  Username (can be NULL)
 * @param ip_addr   IP address (can be NULL)
 * @param details   Additional details
 * @param critical  Is this a critical event?
 * @return          0 on success, -1 on error
 */
int audit_log_event(
    audit_event_type_t type,
    const char *username,
    const char *ip_addr,
    const char *details,
    bool critical);

/*
 * Cleanup audit logging
 */
void audit_log_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_AUDIT_LOG_H */
