/*
 * frc_pacer.h — Frame rate controller / pacer
 *
 * Maintains a target frame rate and computes per-frame deadlines.
 * On each `frc_pacer_tick()` call the pacer decides whether to:
 *
 *   FRC_ACTION_PRESENT   — deliver this frame on time
 *   FRC_ACTION_DROP      — skip this frame (encoder too fast)
 *   FRC_ACTION_DUPLICATE — repeat previous frame (encoder too slow)
 *
 * The pacer uses a simple token-accumulator model: one token is
 * produced every (1e9 / fps) nanoseconds; if tokens > 1 a frame is
 * due, if tokens < 0 the last frame must be duplicated.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_FRC_PACER_H
#define ROOTSTREAM_FRC_PACER_H

#include "frc_clock.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Frame action returned by the pacer */
typedef enum {
    FRC_ACTION_PRESENT   = 0,  /**< Present (send) this frame */
    FRC_ACTION_DROP      = 1,  /**< Drop this frame (rate too high) */
    FRC_ACTION_DUPLICATE = 2,  /**< Duplicate previous (rate too low) */
} frc_action_t;

/** Opaque frame rate controller */
typedef struct frc_pacer_s frc_pacer_t;

/**
 * frc_pacer_create — allocate pacer
 *
 * @param target_fps  Desired output frame rate (e.g. 30.0)
 * @param now_ns      Initial clock reading in nanoseconds
 * @return            Non-NULL handle, or NULL on bad parameters / OOM
 */
frc_pacer_t *frc_pacer_create(double target_fps, uint64_t now_ns);

/**
 * frc_pacer_destroy — free pacer
 *
 * @param p  Pacer to destroy
 */
void frc_pacer_destroy(frc_pacer_t *p);

/**
 * frc_pacer_tick — decide the fate of the current frame
 *
 * @param p       Pacer
 * @param now_ns  Current clock reading in nanoseconds
 * @return        FRC_ACTION_* decision
 */
frc_action_t frc_pacer_tick(frc_pacer_t *p, uint64_t now_ns);

/**
 * frc_pacer_set_fps — update target frame rate (takes effect next tick)
 *
 * @param p          Pacer
 * @param target_fps New target frame rate
 * @return           0 on success, -1 on invalid fps
 */
int frc_pacer_set_fps(frc_pacer_t *p, double target_fps);

/**
 * frc_pacer_target_fps — retrieve current target FPS
 *
 * @param p  Pacer
 * @return   Target FPS, or 0.0 on NULL
 */
double frc_pacer_target_fps(const frc_pacer_t *p);

/**
 * frc_action_name — human-readable action name
 *
 * @param a  Action
 * @return   Static string
 */
const char *frc_action_name(frc_action_t a);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_FRC_PACER_H */
