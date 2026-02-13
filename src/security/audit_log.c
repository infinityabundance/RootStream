/*
 * audit_log.c - Security audit logging implementation
 */

#include "audit_log.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static FILE *g_log_file = NULL;

static const char *event_type_to_string(audit_event_type_t type) {
    switch (type) {
        case AUDIT_EVENT_LOGIN: return "LOGIN";
        case AUDIT_EVENT_LOGOUT: return "LOGOUT";
        case AUDIT_EVENT_LOGIN_FAILED: return "LOGIN_FAILED";
        case AUDIT_EVENT_SESSION_CREATED: return "SESSION_CREATED";
        case AUDIT_EVENT_SESSION_EXPIRED: return "SESSION_EXPIRED";
        case AUDIT_EVENT_KEY_EXCHANGE: return "KEY_EXCHANGE";
        case AUDIT_EVENT_ENCRYPTION_FAILED: return "ENCRYPTION_FAILED";
        case AUDIT_EVENT_SUSPICIOUS_ACTIVITY: return "SUSPICIOUS_ACTIVITY";
        case AUDIT_EVENT_SECURITY_ALERT: return "SECURITY_ALERT";
        default: return "UNKNOWN";
    }
}

/*
 * Initialize audit log
 */
int audit_log_init(const char *log_file) {
    if (log_file) {
        g_log_file = fopen(log_file, "a");
        if (!g_log_file) {
            fprintf(stderr, "ERROR: Failed to open audit log: %s\n", log_file);
            return -1;
        }
    } else {
        g_log_file = stderr;
    }
    
    audit_log_event(AUDIT_EVENT_SECURITY_ALERT, NULL, NULL,
                   "Audit logging initialized", false);
    return 0;
}

/*
 * Log event
 */
int audit_log_event(
    audit_event_type_t type,
    const char *username,
    const char *ip_addr,
    const char *details,
    bool critical)
{
    if (!g_log_file) {
        g_log_file = stderr;
    }
    
    /* Get timestamp */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    /* Log format: [TIMESTAMP] [SEVERITY] EVENT_TYPE user=USERNAME ip=IP_ADDR details */
    fprintf(g_log_file, "[%s] [%s] %s",
            timestamp,
            critical ? "CRITICAL" : "INFO",
            event_type_to_string(type));
    
    if (username) {
        fprintf(g_log_file, " user=%s", username);
    }
    
    if (ip_addr) {
        fprintf(g_log_file, " ip=%s", ip_addr);
    }
    
    if (details) {
        fprintf(g_log_file, " details=%s", details);
    }
    
    fprintf(g_log_file, "\n");
    fflush(g_log_file);
    
    return 0;
}

/*
 * Cleanup
 */
void audit_log_cleanup(void) {
    if (g_log_file && g_log_file != stderr) {
        audit_log_event(AUDIT_EVENT_SECURITY_ALERT, NULL, NULL,
                       "Audit logging shutdown", false);
        fclose(g_log_file);
        g_log_file = NULL;
    }
}
