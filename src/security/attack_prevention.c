/*
 * attack_prevention.c - Attack prevention implementation
 */

#include "attack_prevention.h"
#include "crypto_primitives.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

#define MAX_NONCES 1024
#define MAX_FAILED_ATTEMPTS 256
#define MAX_RATE_LIMIT_ENTRIES 256
#define LOCKOUT_THRESHOLD 5
#define LOCKOUT_DURATION_SEC 300

/* Nonce cache for replay prevention */
static struct {
    uint8_t nonce[32];
    bool used;
} g_nonce_cache[MAX_NONCES];
static int g_nonce_count = 0;

/* Brute force tracker */
static struct {
    char username[64];
    uint32_t failed_attempts;
    uint64_t lockout_until_us;
} g_failed_attempts[MAX_FAILED_ATTEMPTS];
static int g_failed_count = 0;

/* Rate limiter */
static struct {
    char client_id[128];
    uint32_t request_count;
    uint64_t window_start_us;
} g_rate_limits[MAX_RATE_LIMIT_ENTRIES];
static int g_rate_limit_count = 0;

/*
 * Initialize attack prevention
 */
int attack_prevention_init(void) {
    g_nonce_count = 0;
    g_failed_count = 0;
    g_rate_limit_count = 0;
    memset(g_nonce_cache, 0, sizeof(g_nonce_cache));
    memset(g_failed_attempts, 0, sizeof(g_failed_attempts));
    memset(g_rate_limits, 0, sizeof(g_rate_limits));
    return 0;
}

/*
 * Check nonce (replay attack prevention)
 */
bool attack_prevention_check_nonce(const uint8_t *nonce, size_t nonce_len) {
    if (!nonce || nonce_len == 0) {
        return false;
    }
    
    /* Check if nonce already used */
    for (int i = 0; i < g_nonce_count && i < MAX_NONCES; i++) {
        if (g_nonce_cache[i].used &&
            crypto_prim_constant_time_compare(g_nonce_cache[i].nonce, nonce,
                                             nonce_len < 32 ? nonce_len : 32)) {
            return false;  /* Replay detected */
        }
    }
    
    /* Add to cache */
    if (g_nonce_count < MAX_NONCES) {
        memcpy(g_nonce_cache[g_nonce_count].nonce, nonce,
               nonce_len < 32 ? nonce_len : 32);
        g_nonce_cache[g_nonce_count].used = true;
        g_nonce_count++;
    } else {
        /* Cache full, replace oldest (FIFO) */
        memmove(&g_nonce_cache[0], &g_nonce_cache[1],
                sizeof(g_nonce_cache[0]) * (MAX_NONCES - 1));
        memcpy(g_nonce_cache[MAX_NONCES - 1].nonce, nonce,
               nonce_len < 32 ? nonce_len : 32);
        g_nonce_cache[MAX_NONCES - 1].used = true;
    }
    
    return true;
}

/*
 * Record failed login
 */
int attack_prevention_record_failed_login(const char *username) {
    if (!username) {
        return -1;
    }
    
    /* Find existing entry */
    for (int i = 0; i < g_failed_count; i++) {
        if (strcmp(g_failed_attempts[i].username, username) == 0) {
            g_failed_attempts[i].failed_attempts++;
            
            /* Lock account if threshold exceeded */
            if (g_failed_attempts[i].failed_attempts >= LOCKOUT_THRESHOLD) {
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                uint64_t now_us = (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
                g_failed_attempts[i].lockout_until_us = now_us +
                                                        LOCKOUT_DURATION_SEC * 1000000;
            }
            return 0;
        }
    }
    
    /* Add new entry */
    if (g_failed_count < MAX_FAILED_ATTEMPTS) {
        strncpy(g_failed_attempts[g_failed_count].username, username, 63);
        g_failed_attempts[g_failed_count].username[63] = '\0';
        g_failed_attempts[g_failed_count].failed_attempts = 1;
        g_failed_attempts[g_failed_count].lockout_until_us = 0;
        g_failed_count++;
    }
    
    return 0;
}

/*
 * Check if account locked
 */
bool attack_prevention_is_account_locked(const char *username) {
    if (!username) {
        return false;
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t now_us = (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
    
    for (int i = 0; i < g_failed_count; i++) {
        if (strcmp(g_failed_attempts[i].username, username) == 0) {
            if (g_failed_attempts[i].lockout_until_us > now_us) {
                return true;  /* Account locked */
            }
        }
    }
    
    return false;
}

/*
 * Reset failed attempts
 */
int attack_prevention_reset_failed_attempts(const char *username) {
    if (!username) {
        return -1;
    }
    
    for (int i = 0; i < g_failed_count; i++) {
        if (strcmp(g_failed_attempts[i].username, username) == 0) {
            g_failed_attempts[i].failed_attempts = 0;
            g_failed_attempts[i].lockout_until_us = 0;
            return 0;
        }
    }
    
    return 0;
}

/*
 * Check rate limiting
 */
bool attack_prevention_is_rate_limited(const char *client_id, uint32_t max_per_min) {
    if (!client_id) {
        return false;
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t now_us = (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
    uint64_t window_us = 60 * 1000000;  /* 1 minute */
    
    /* Find existing entry */
    for (int i = 0; i < g_rate_limit_count; i++) {
        if (strcmp(g_rate_limits[i].client_id, client_id) == 0) {
            /* Check if window expired */
            if (now_us - g_rate_limits[i].window_start_us > window_us) {
                /* Reset window */
                g_rate_limits[i].request_count = 1;
                g_rate_limits[i].window_start_us = now_us;
                return false;
            }
            
            /* Increment counter */
            g_rate_limits[i].request_count++;
            
            /* Check limit */
            return g_rate_limits[i].request_count > max_per_min;
        }
    }
    
    /* Add new entry */
    if (g_rate_limit_count < MAX_RATE_LIMIT_ENTRIES) {
        strncpy(g_rate_limits[g_rate_limit_count].client_id, client_id, 127);
        g_rate_limits[g_rate_limit_count].client_id[127] = '\0';
        g_rate_limits[g_rate_limit_count].request_count = 1;
        g_rate_limits[g_rate_limit_count].window_start_us = now_us;
        g_rate_limit_count++;
    }
    
    return false;
}

/*
 * Cleanup
 */
void attack_prevention_cleanup(void) {
    crypto_prim_secure_wipe(g_nonce_cache, sizeof(g_nonce_cache));
    crypto_prim_secure_wipe(g_failed_attempts, sizeof(g_failed_attempts));
    memset(g_rate_limits, 0, sizeof(g_rate_limits));
    g_nonce_count = 0;
    g_failed_count = 0;
    g_rate_limit_count = 0;
}
