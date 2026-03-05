/*
 * rm_entry.h — Retry Manager: per-request retry entry
 *
 * Tracks retry state for a single outstanding request: the unique
 * request ID, how many attempts have been made, when the next retry
 * should fire, the maximum allowed attempts, and the base delay used
 * for exponential back-off.
 *
 * Back-off formula:
 *   next_retry_us += base_delay_us × 2^(attempt_count - 1)
 * capped at RM_MAX_BACKOFF_US.
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_RM_ENTRY_H
#define ROOTSTREAM_RM_ENTRY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RM_MAX_BACKOFF_US  30000000ULL  /**< Hard cap on inter-retry delay (30 s) */

/** Retry state for one request */
typedef struct {
    uint64_t request_id;      /**< Unique request identifier */
    uint32_t attempt_count;   /**< Attempts made so far (0 = not yet tried) */
    uint32_t max_attempts;    /**< Maximum attempts before giving up */
    uint64_t base_delay_us;   /**< Initial back-off delay (µs) */
    uint64_t next_retry_us;   /**< Wall-clock µs when next attempt is due */
    bool     in_use;
} rm_entry_t;

/**
 * rm_entry_init — initialise a retry entry
 *
 * @param e             Entry
 * @param request_id    Unique request ID
 * @param now_us        Current wall-clock µs
 * @param base_delay_us Initial back-off interval (µs); first attempt fires
 *                      after this delay (pass 0 to fire immediately)
 * @param max_attempts  Maximum attempts (> 0)
 * @return              0 on success, -1 on NULL or invalid params
 */
int rm_entry_init(rm_entry_t *e,
                  uint64_t    request_id,
                  uint64_t    now_us,
                  uint64_t    base_delay_us,
                  uint32_t    max_attempts);

/**
 * rm_entry_advance — record one attempt and compute next_retry_us
 *
 * @param e       Entry
 * @param now_us  Current wall-clock µs
 * @return        true if more attempts remain, false if max reached
 */
bool rm_entry_advance(rm_entry_t *e, uint64_t now_us);

/**
 * rm_entry_is_due — check if this entry is ready to retry
 *
 * @param e       Entry
 * @param now_us  Current wall-clock µs
 * @return        true if now_us >= next_retry_us
 */
bool rm_entry_is_due(const rm_entry_t *e, uint64_t now_us);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_RM_ENTRY_H */
