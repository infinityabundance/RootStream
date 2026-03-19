/*
 * abr_controller.h — ABR decision engine
 *
 * Combines bandwidth estimation and ladder selection to produce
 * streaming quality decisions.  Implements a conservative ABR
 * algorithm with hysteresis to avoid rapid quality oscillation:
 *
 *   - Downgrade when estimated bandwidth < current_bitrate * SAFETY_MARGIN
 *   - Upgrade after consecutive stable periods (ABR_UPGRADE_HOLD)
 *   - Hysteresis prevents toggling between adjacent levels
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_ABR_CONTROLLER_H
#define ROOTSTREAM_ABR_CONTROLLER_H

#include <stdbool.h>

#include "abr_estimator.h"
#include "abr_ladder.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Safety margin: fraction of estimated BW used for budgeting */
#define ABR_SAFETY_MARGIN 0.85f

/** Consecutive stable ticks required before upgrading */
#define ABR_UPGRADE_HOLD_TICKS 3

/** Result returned by abr_controller_tick */
typedef struct {
    int new_level_idx;  /**< Selected level index */
    bool level_changed; /**< True if level is different from previous */
    bool is_downgrade;  /**< True if we moved to a lower level */
} abr_decision_t;

/** Opaque ABR controller */
typedef struct abr_controller_s abr_controller_t;

/**
 * abr_controller_create — allocate controller
 *
 * @param estimator  Bandwidth estimator (borrowed, not owned)
 * @param ladder     Quality ladder (borrowed, not owned)
 * @return           Non-NULL handle, or NULL on error
 */
abr_controller_t *abr_controller_create(abr_estimator_t *estimator, abr_ladder_t *ladder);

/**
 * abr_controller_destroy — free controller
 *
 * @param ctrl  Controller to destroy
 */
void abr_controller_destroy(abr_controller_t *ctrl);

/**
 * abr_controller_tick — make a quality decision based on current BW estimate
 *
 * Call once per segment or decision interval.
 *
 * @param ctrl  Controller
 * @param out   Decision output
 * @return      0 on success, -1 on error
 */
int abr_controller_tick(abr_controller_t *ctrl, abr_decision_t *out);

/**
 * abr_controller_current_level — currently selected level index
 *
 * @param ctrl  Controller
 * @return      Level index, or -1 on error
 */
int abr_controller_current_level(const abr_controller_t *ctrl);

/**
 * abr_controller_force_level — override ABR and pin to @level_idx
 *
 * Useful for manual quality selection.
 *
 * @param ctrl       Controller
 * @param level_idx  Level to force (-1 = release override)
 * @return           0 on success, -1 on bad index
 */
int abr_controller_force_level(abr_controller_t *ctrl, int level_idx);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_ABR_CONTROLLER_H */
