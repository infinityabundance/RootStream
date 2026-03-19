/*
 * rate_limiter.h — Per-viewer token bucket rate limiter registry
 *
 * Maintains a map of viewer_id → token_bucket_t so each viewer gets
 * independent rate limiting.  Up to RATE_LIMITER_MAX_VIEWERS entries.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_RATE_LIMITER_H
#define ROOTSTREAM_RATE_LIMITER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "token_bucket.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RATE_LIMITER_MAX_VIEWERS 256

/** Opaque rate limiter registry */
typedef struct rate_limiter_s rate_limiter_t;

/**
 * rate_limiter_create — allocate registry with default rate/burst
 *
 * @param default_rate_bps  Default refill rate applied to new viewers
 * @param default_burst     Default burst capacity for new viewers
 * @return                  Non-NULL handle, or NULL on error
 */
rate_limiter_t *rate_limiter_create(double default_rate_bps, double default_burst);

/**
 * rate_limiter_destroy — free all buckets and the registry
 *
 * @param rl  Registry to destroy
 */
void rate_limiter_destroy(rate_limiter_t *rl);

/**
 * rate_limiter_add_viewer — register a new viewer
 *
 * If @viewer_id already exists, this is a no-op (returns 0).
 *
 * @param rl         Registry
 * @param viewer_id  Unique viewer identifier
 * @param now_us     Current time in µs
 * @return           0 on success, -1 if registry full / NULL args
 */
int rate_limiter_add_viewer(rate_limiter_t *rl, uint64_t viewer_id, uint64_t now_us);

/**
 * rate_limiter_remove_viewer — unregister a viewer
 *
 * @param rl         Registry
 * @param viewer_id  Viewer to remove
 * @return           0 on success, -1 if not found
 */
int rate_limiter_remove_viewer(rate_limiter_t *rl, uint64_t viewer_id);

/**
 * rate_limiter_consume — consume @n bytes for @viewer_id
 *
 * @param rl         Registry
 * @param viewer_id  Viewer
 * @param bytes      Packet size in bytes
 * @param now_us     Current time in µs
 * @return           true if allowed, false if throttled or viewer not found
 */
bool rate_limiter_consume(rate_limiter_t *rl, uint64_t viewer_id, double bytes, uint64_t now_us);

/**
 * rate_limiter_viewer_count — number of registered viewers
 *
 * @param rl  Registry
 * @return    Count
 */
size_t rate_limiter_viewer_count(const rate_limiter_t *rl);

/**
 * rate_limiter_has_viewer — return true if @viewer_id is registered
 *
 * @param rl         Registry
 * @param viewer_id  Viewer to look up
 * @return           true if registered
 */
bool rate_limiter_has_viewer(const rate_limiter_t *rl, uint64_t viewer_id);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_RATE_LIMITER_H */
