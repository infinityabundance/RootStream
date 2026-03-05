/*
 * fr_target.h — Target FPS tracker with running average interval
 *
 * Tracks the actual frame rate achieved by maintaining a running
 * average of inter-frame intervals.  Call `fr_target_mark()` whenever
 * a frame is produced; the tracker computes actual_fps from the
 * exponentially-weighted moving average of interval_us.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_FR_TARGET_H
#define ROOTSTREAM_FR_TARGET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FR_TARGET_EWMA_ALPHA  0.1   /**< EWMA smoothing factor */

/** Target FPS tracker */
typedef struct {
    double   target_fps;       /**< Configured target fps */
    double   avg_interval_us;  /**< EWMA of inter-frame interval (µs) */
    double   actual_fps;       /**< Computed actual fps = 1e6/avg_interval_us */
    uint64_t last_mark_us;     /**< Timestamp of last mark (µs) */
    uint64_t frame_count;      /**< Total frames marked */
    int      initialised;
} fr_target_t;

/**
 * fr_target_init — initialise tracker
 *
 * @param t          Tracker
 * @param target_fps Target frame rate (> 0)
 * @return           0 on success, -1 on NULL or invalid
 */
int fr_target_init(fr_target_t *t, double target_fps);

/**
 * fr_target_mark — record a new frame at time now_us
 *
 * @param t      Tracker
 * @param now_us Current time in µs
 * @return       0 on success, -1 on NULL
 */
int fr_target_mark(fr_target_t *t, uint64_t now_us);

/**
 * fr_target_reset — clear tracking state
 *
 * @param t  Tracker
 */
void fr_target_reset(fr_target_t *t);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_FR_TARGET_H */
