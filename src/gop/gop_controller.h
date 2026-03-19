/*
 * gop_controller.h — Adaptive GOP controller
 *
 * On each frame the caller provides:
 *   - scene_score  [0.0, 1.0]: perceptual change since last frame
 *   - rtt_us:  current smoothed RTT
 *   - loss:    current loss fraction [0.0, 1.0]
 *
 * The controller returns whether an IDR should be forced now.
 *
 * Decision rules (checked in order):
 *   1. Minimum cooldown: never force IDR within min_gop_frames of last IDR.
 *   2. Maximum interval: always force IDR at max_gop_frames.
 *   3. Scene change: force IDR if scene_score >= scene_change_threshold.
 *   4. Loss recovery: force IDR if loss >= loss_threshold AND
 *      rtt_us < rtt_threshold_us (don't add IDR pressure during high RTT).
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_GOP_CONTROLLER_H
#define ROOTSTREAM_GOP_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

#include "gop_policy.h"

#ifdef __cplusplus
extern "C" {
#endif

/** IDR decision */
typedef enum {
    GOP_DECISION_P_FRAME = 0, /**< Encode as P/B frame */
    GOP_DECISION_IDR = 1,     /**< Force IDR (keyframe) */
} gop_decision_t;

/** Reason for forced IDR */
typedef enum {
    GOP_REASON_NATURAL = 0,       /**< max_gop interval reached */
    GOP_REASON_SCENE_CHANGE = 1,  /**< Scene-change score exceeded threshold */
    GOP_REASON_LOSS_RECOVERY = 2, /**< Loss-driven recovery IDR */
    GOP_REASON_NONE = 3,          /**< Not an IDR */
} gop_reason_t;

/** Opaque GOP controller */
typedef struct gop_controller_s gop_controller_t;

/**
 * gop_controller_create — allocate controller
 *
 * @param policy  Policy parameters (copied)
 * @return        Non-NULL handle, or NULL on error
 */
gop_controller_t *gop_controller_create(const gop_policy_t *policy);

/**
 * gop_controller_destroy — free controller
 *
 * @param gc  Controller to destroy
 */
void gop_controller_destroy(gop_controller_t *gc);

/**
 * gop_controller_update_policy — replace policy
 *
 * @param gc      Controller
 * @param policy  New policy
 * @return        0 on success, -1 on invalid policy
 */
int gop_controller_update_policy(gop_controller_t *gc, const gop_policy_t *policy);

/**
 * gop_controller_next_frame — decide IDR for the next frame
 *
 * @param gc          Controller
 * @param scene_score Perceptual change score [0.0, 1.0]
 * @param rtt_us      Current smoothed RTT in µs
 * @param loss        Current loss fraction [0.0, 1.0]
 * @param reason_out  If non-NULL, set to the IDR reason (NONE for P-frame)
 * @return            GOP_DECISION_IDR or GOP_DECISION_P_FRAME
 */
gop_decision_t gop_controller_next_frame(gop_controller_t *gc, float scene_score, uint64_t rtt_us,
                                         float loss, gop_reason_t *reason_out);

/**
 * gop_controller_force_idr — inject an external IDR (e.g. from PLI request)
 *
 * Resets the cooldown counter as if an IDR had been issued now.
 *
 * @param gc  Controller
 */
void gop_controller_force_idr(gop_controller_t *gc);

/**
 * gop_controller_frames_since_idr — frames elapsed since last IDR
 *
 * @param gc  Controller
 * @return    Frame count
 */
int gop_controller_frames_since_idr(const gop_controller_t *gc);

/**
 * gop_decision_name — human-readable decision name
 *
 * @param d  Decision
 * @return   Static string
 */
const char *gop_decision_name(gop_decision_t d);

/**
 * gop_reason_name — human-readable reason name
 *
 * @param r  Reason
 * @return   Static string
 */
const char *gop_reason_name(gop_reason_t r);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_GOP_CONTROLLER_H */
