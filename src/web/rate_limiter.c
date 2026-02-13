/**
 * PHASE 19: Web Dashboard - Rate Limiter Implementation
 */

#include "rate_limiter.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define MAX_CLIENTS 1000
#define WINDOW_SECONDS 60

typedef struct {
    char client_ip[64];
    uint32_t request_count;
    uint64_t window_start;
} client_limit_entry_t;

struct rate_limiter {
    uint32_t requests_per_minute;
    client_limit_entry_t entries[MAX_CLIENTS];
    int entry_count;
    pthread_mutex_t lock;
};

/**
 * Get current time in seconds
 */
static uint64_t get_current_time(void) {
    return (uint64_t)time(NULL);
}

/**
 * Initialize rate limiter
 */
rate_limiter_t *rate_limiter_init(uint32_t requests_per_minute) {
    rate_limiter_t *limiter = (rate_limiter_t *)calloc(1, sizeof(rate_limiter_t));
    if (!limiter) {
        return NULL;
    }

    limiter->requests_per_minute = requests_per_minute;
    limiter->entry_count = 0;
    pthread_mutex_init(&limiter->lock, NULL);

    return limiter;
}

/**
 * Check if client is rate limited
 */
bool rate_limiter_is_limited(rate_limiter_t *limiter, const char *client_ip) {
    if (!limiter || !client_ip) {
        return false;
    }

    pthread_mutex_lock(&limiter->lock);

    uint64_t now = get_current_time();

    // Find existing entry
    for (int i = 0; i < limiter->entry_count; i++) {
        if (strcmp(limiter->entries[i].client_ip, client_ip) == 0) {
            // Check if window has expired
            if (now - limiter->entries[i].window_start >= WINDOW_SECONDS) {
                // Reset window
                limiter->entries[i].request_count = 1;
                limiter->entries[i].window_start = now;
                pthread_mutex_unlock(&limiter->lock);
                return false;
            }

            // Increment count
            limiter->entries[i].request_count++;

            // Check limit
            bool is_limited = (limiter->entries[i].request_count > limiter->requests_per_minute);
            pthread_mutex_unlock(&limiter->lock);
            return is_limited;
        }
    }

    // New client - add entry
    if (limiter->entry_count < MAX_CLIENTS) {
        client_limit_entry_t *entry = &limiter->entries[limiter->entry_count];
        strncpy(entry->client_ip, client_ip, sizeof(entry->client_ip) - 1);
        entry->request_count = 1;
        entry->window_start = now;
        limiter->entry_count++;
    }

    pthread_mutex_unlock(&limiter->lock);
    return false;
}

/**
 * Reset rate limit for client
 */
int rate_limiter_reset(rate_limiter_t *limiter, const char *client_ip) {
    if (!limiter || !client_ip) {
        return -1;
    }

    pthread_mutex_lock(&limiter->lock);

    for (int i = 0; i < limiter->entry_count; i++) {
        if (strcmp(limiter->entries[i].client_ip, client_ip) == 0) {
            limiter->entries[i].request_count = 0;
            limiter->entries[i].window_start = get_current_time();
            pthread_mutex_unlock(&limiter->lock);
            return 0;
        }
    }

    pthread_mutex_unlock(&limiter->lock);
    return -1;
}

/**
 * Cleanup rate limiter
 */
void rate_limiter_cleanup(rate_limiter_t *limiter) {
    if (!limiter) {
        return;
    }

    pthread_mutex_destroy(&limiter->lock);
    free(limiter);
}
