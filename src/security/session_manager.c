/*
 * session_manager.c - Session management implementation
 */

#include "session_manager.h"
#include "crypto_primitives.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

#define MAX_SESSIONS 256
#define MAX_USERNAME 64

typedef struct {
    char session_id[SESSION_ID_LEN];
    char username[MAX_USERNAME];
    uint64_t creation_time_us;
    uint64_t expiration_time_us;
    bool is_active;
    uint8_t session_secret[32];
} session_t;

static session_t g_sessions[MAX_SESSIONS];
static int g_session_count = 0;
static uint32_t g_timeout_sec = 3600;

/*
 * Initialize session manager
 */
int session_manager_init(uint32_t timeout_sec) {
    g_session_count = 0;
    g_timeout_sec = timeout_sec > 0 ? timeout_sec : 3600;
    memset(g_sessions, 0, sizeof(g_sessions));
    return crypto_prim_init();
}

/*
 * Create session
 */
int session_manager_create(const char *username, char *session_id) {
    if (!username || !session_id || g_session_count >= MAX_SESSIONS) {
        return -1;
    }
    
    /* Find available slot */
    int slot = -1;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!g_sessions[i].is_active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;
    }
    
    /* Generate session ID */
    uint8_t id_bytes[32];
    crypto_prim_random_bytes(id_bytes, sizeof(id_bytes));
    
    const char hex[] = "0123456789abcdef";
    for (int i = 0; i < 32; i++) {
        g_sessions[slot].session_id[i * 2] = hex[(id_bytes[i] >> 4) & 0xF];
        g_sessions[slot].session_id[i * 2 + 1] = hex[id_bytes[i] & 0xF];
    }
    g_sessions[slot].session_id[64] = '\0';
    
    /* Copy session ID to output */
    strcpy(session_id, g_sessions[slot].session_id);
    
    /* Set session info */
    strncpy(g_sessions[slot].username, username, MAX_USERNAME - 1);
    g_sessions[slot].username[MAX_USERNAME - 1] = '\0';
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    g_sessions[slot].creation_time_us = (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
    g_sessions[slot].expiration_time_us = g_sessions[slot].creation_time_us + 
                                          (uint64_t)g_timeout_sec * 1000000;
    g_sessions[slot].is_active = true;
    
    /* Generate session secret for PFS */
    crypto_prim_random_bytes(g_sessions[slot].session_secret, 32);
    
    g_session_count++;
    return 0;
}

/*
 * Validate session
 */
bool session_manager_is_valid(const char *session_id) {
    if (!session_id) {
        return false;
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t now_us = (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
    
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (g_sessions[i].is_active &&
            strcmp(g_sessions[i].session_id, session_id) == 0) {
            return now_us < g_sessions[i].expiration_time_us;
        }
    }
    
    return false;
}

/*
 * Refresh session
 */
int session_manager_refresh(const char *session_id) {
    if (!session_id) {
        return -1;
    }
    
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (g_sessions[i].is_active &&
            strcmp(g_sessions[i].session_id, session_id) == 0) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            uint64_t now_us = (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
            g_sessions[i].expiration_time_us = now_us + (uint64_t)g_timeout_sec * 1000000;
            return 0;
        }
    }
    
    return -1;
}

/*
 * Invalidate session
 */
int session_manager_invalidate(const char *session_id) {
    if (!session_id) {
        return -1;
    }
    
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (g_sessions[i].is_active &&
            strcmp(g_sessions[i].session_id, session_id) == 0) {
            /* Mark as inactive first */
            g_sessions[i].is_active = false;
            
            /* Securely wipe sensitive session data */
            crypto_prim_secure_wipe(g_sessions[i].session_secret, 32);
            crypto_prim_secure_wipe(g_sessions[i].session_id, SESSION_ID_LEN);
            
            g_session_count--;
            return 0;
        }
    }
    
    return -1;
}

/*
 * Cleanup expired sessions
 */
int session_manager_cleanup_expired(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t now_us = (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
    
    int cleaned = 0;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (g_sessions[i].is_active &&
            now_us >= g_sessions[i].expiration_time_us) {
            /* Mark as inactive first */
            g_sessions[i].is_active = false;
            
            /* Securely wipe sensitive session data */
            crypto_prim_secure_wipe(g_sessions[i].session_secret, 32);
            crypto_prim_secure_wipe(g_sessions[i].session_id, SESSION_ID_LEN);
            
            g_session_count--;
            cleaned++;
        }
    }
    
    return cleaned;
}

/*
 * Cleanup
 */
void session_manager_cleanup(void) {
    crypto_prim_secure_wipe(g_sessions, sizeof(g_sessions));
    g_session_count = 0;
}
