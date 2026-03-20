/*
 * kfr_handler.h — Keyframe request deduplication and rate limiter
 *
 * Prevents keyframe request floods by enforcing a minimum inter-request
 * interval (cooldown) per SSRC.  Duplicate requests arriving within the
 * cooldown window are silently dropped; the first request after the
 * cooldown is forwarded.
 *
 * Time is caller-supplied (µs) for testability.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_KFR_HANDLER_H
#define ROOTSTREAM_KFR_HANDLER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "kfr_message.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum tracked SSRCs */
#define KFR_MAX_SSRC 64

/** Default minimum interval between forwarded requests per SSRC (250 ms) */
#define KFR_DEFAULT_COOLDOWN_US 250000ULL

/** Handler decision */
typedef enum {
    KFR_DECISION_FORWARD = 0,  /**< Forward request to encoder */
    KFR_DECISION_SUPPRESS = 1, /**< Suppress (duplicate / too soon) */
} kfr_decision_t;

/** Opaque keyframe request handler */
typedef struct kfr_handler_s kfr_handler_t;

/**
 * kfr_handler_create — allocate handler
 *
 * @param cooldown_us  Minimum µs between forwarded requests per SSRC
 * @return             Non-NULL handle, or NULL on error
 */
kfr_handler_t *kfr_handler_create(uint64_t cooldown_us);

/**
 * kfr_handler_destroy — free handler
 *
 * @param h  Handler to destroy
 */
void kfr_handler_destroy(kfr_handler_t *h);

/**
 * kfr_handler_submit — submit an incoming keyframe request
 *
 * @param h       Handler
 * @param msg     Incoming request
 * @param now_us  Current time in µs
 * @return        FORWARD or SUPPRESS
 */
kfr_decision_t kfr_handler_submit(kfr_handler_t *h, const kfr_message_t *msg, uint64_t now_us);

/**
 * kfr_handler_flush_ssrc — forcibly reset cooldown for @ssrc
 *
 * Allows the next request for this SSRC to be forwarded immediately.
 *
 * @param h     Handler
 * @param ssrc  SSRC to flush
 */
void kfr_handler_flush_ssrc(kfr_handler_t *h, uint32_t ssrc);

/**
 * kfr_handler_set_cooldown — update the cooldown window
 *
 * @param h           Handler
 * @param cooldown_us New cooldown in µs
 * @return            0 on success, -1 on NULL
 */
int kfr_handler_set_cooldown(kfr_handler_t *h, uint64_t cooldown_us);

/**
 * kfr_decision_name — human-readable decision name
 *
 * @param d  Decision
 * @return   Static string
 */
const char *kfr_decision_name(kfr_decision_t d);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_KFR_HANDLER_H */
