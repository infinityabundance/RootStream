/**
 * PHASE 19: Web Dashboard - Rate Limiter
 * 
 * Provides rate limiting for API requests
 */

#ifndef RATE_LIMITER_H
#define RATE_LIMITER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Rate limiter handle
 */
typedef struct rate_limiter rate_limiter_t;

/**
 * Initialize rate limiter
 */
rate_limiter_t *rate_limiter_init(uint32_t requests_per_minute);

/**
 * Check if client is rate limited
 */
bool rate_limiter_is_limited(rate_limiter_t *limiter, const char *client_ip);

/**
 * Reset rate limit for client
 */
int rate_limiter_reset(rate_limiter_t *limiter, const char *client_ip);

/**
 * Cleanup rate limiter
 */
void rate_limiter_cleanup(rate_limiter_t *limiter);

#ifdef __cplusplus
}
#endif

#endif // RATE_LIMITER_H
