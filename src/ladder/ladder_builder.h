/*
 * ladder_builder.h — ABR bitrate ladder builder
 *
 * Constructs an ascending bitrate ladder of up to LADDER_MAX_RUNGS
 * renditions from a maximum bitrate, a minimum bitrate floor, and a
 * step-down ratio.  Each successive rung has bitrate reduced by
 * `step_ratio` (e.g. 0.5 halves each step).
 *
 * Resolution and frame rate are scaled proportionally: resolution
 * tracks the nearest standard height in `std_heights[]`, and fps is
 * halved when bitrate drops below `fps_reduce_threshold`.
 *
 * Thread-safety: stateless builder function — thread-safe.
 */

#ifndef ROOTSTREAM_LADDER_BUILDER_H
#define ROOTSTREAM_LADDER_BUILDER_H

#include "ladder_rung.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LADDER_MAX_RUNGS  8   /**< Maximum rungs produced */

/** Ladder build parameters */
typedef struct {
    uint32_t max_bps;              /**< Highest rung bitrate (bits/sec) */
    uint32_t min_bps;              /**< Lowest rung floor (bits/sec) */
    float    step_ratio;           /**< Reduction per rung (0 < r < 1) */
    uint16_t max_height;           /**< Height at max bitrate (pixels) */
    float    max_fps;              /**< FPS at max bitrate */
    float    fps_reduce_threshold; /**< Bitrate below which fps is halved */
} ladder_params_t;

/**
 * ladder_build — produce an ascending bitrate ladder
 *
 * @param p      Build parameters
 * @param rungs  Output array of at least LADDER_MAX_RUNGS entries
 * @param n_out  Number of rungs written
 * @return       0 on success, -1 on NULL or invalid params
 */
int ladder_build(const ladder_params_t *p,
                   ladder_rung_t         *rungs,
                   int                   *n_out);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_LADDER_BUILDER_H */
