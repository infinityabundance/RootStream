/*
 * gop_policy.h — Adaptive GOP controller policy parameters
 *
 * Encapsulates the tunable knobs that govern when the GOP controller
 * forces an IDR frame:
 *   - min_gop_frames: minimum inter-IDR interval (to avoid IDR spam)
 *   - max_gop_frames: maximum inter-IDR interval (keyframe recovery bound)
 *   - scene_change_threshold: scene-score [0.0, 1.0] above which an IDR
 *     is forced irrespective of network conditions
 *   - rtt_threshold_us: RTT above which congestion-driven IDRs are
 *     suppressed (reduce retransmission pressure)
 *   - loss_threshold: loss fraction above which the GOP is shortened
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_GOP_POLICY_H
#define ROOTSTREAM_GOP_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Default policy constants */
#define GOP_DEFAULT_MIN_FRAMES 15              /**< 0.5 s at 30 fps */
#define GOP_DEFAULT_MAX_FRAMES 300             /**< 10 s at 30 fps */
#define GOP_DEFAULT_SCENE_THRESHOLD 0.8f       /**< Scene-change score */
#define GOP_DEFAULT_RTT_THRESHOLD_US 200000ULL /**< 200 ms */
#define GOP_DEFAULT_LOSS_THRESHOLD 0.02f       /**< 2% loss */

/** GOP policy */
typedef struct {
    int min_gop_frames;           /**< Min frames between forced IDRs */
    int max_gop_frames;           /**< Max frames before natural IDR */
    float scene_change_threshold; /**< [0.0, 1.0] scene-change score */
    uint64_t rtt_threshold_us;    /**< RTT (µs) above which = suppress */
    float loss_threshold;         /**< Loss fraction above which shorten GOP */
} gop_policy_t;

/**
 * gop_policy_default — fill @p with default policy values
 *
 * @param p  Policy to initialise
 * @return   0 on success, -1 on NULL
 */
int gop_policy_default(gop_policy_t *p);

/**
 * gop_policy_validate — check that policy parameters are self-consistent
 *
 * @param p  Policy to validate
 * @return   0 if valid, -1 if any field is out of range or inconsistent
 */
int gop_policy_validate(const gop_policy_t *p);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_GOP_POLICY_H */
