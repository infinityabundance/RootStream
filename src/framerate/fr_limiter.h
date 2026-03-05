/*
 * fr_limiter.h — Token-bucket frame rate limiter
 *
 * Implements a token-bucket algorithm for frame pacing.  The bucket
 * accumulates tokens at the target frame rate; one token is consumed
 * per frame.  `fr_limiter_tick()` advances the bucket by `elapsed_us`
 * and returns the number of frames that may be emitted.
 *
 * Tokens are capped at max_burst (default = 2) to prevent a burst of
 * frames after a long pause.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_FR_LIMITER_H
#define ROOTSTREAM_FR_LIMITER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FR_MAX_BURST  2   /**< Maximum token accumulation (frames) */

/** Token-bucket frame limiter */
typedef struct {
    double   target_fps;      /**< Target frame rate (frames/second) */
    double   tokens;          /**< Current token count (fractional) */
    int      max_burst;       /**< Token cap (default FR_MAX_BURST) */
} fr_limiter_t;

/**
 * fr_limiter_init — initialise limiter
 *
 * @param l          Limiter to initialise
 * @param target_fps Target frame rate (must be > 0)
 * @return           0 on success, -1 on NULL or invalid fps
 */
int fr_limiter_init(fr_limiter_t *l, double target_fps);

/**
 * fr_limiter_tick — advance token bucket by elapsed_us microseconds
 *
 * @param l          Limiter
 * @param elapsed_us Time elapsed since last tick (µs)
 * @return           Number of frames that may be emitted (≥ 0)
 */
int fr_limiter_tick(fr_limiter_t *l, uint64_t elapsed_us);

/**
 * fr_limiter_reset — drain tokens and restart bucket
 *
 * @param l  Limiter
 */
void fr_limiter_reset(fr_limiter_t *l);

/**
 * fr_limiter_set_fps — update target fps at runtime
 *
 * @param l    Limiter
 * @param fps  New target (must be > 0)
 * @return     0 on success, -1 on invalid fps
 */
int fr_limiter_set_fps(fr_limiter_t *l, double fps);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_FR_LIMITER_H */
