/*
 * scene_detector.h — Histogram-based scene-change detection
 *
 * Detects cuts and gradual transitions by comparing consecutive frame
 * luma histograms.  A "scene change" is declared when the histogram
 * difference exceeds a configurable threshold.
 *
 * Designed to be inserted into the encoder pipeline before keyframe
 * decisions: a scene change forces an IDR frame regardless of the
 * regular keyframe interval.
 *
 * Thread-safety: each scene_detector_t is NOT thread-safe; use one
 * instance per encoding thread.
 */

#ifndef ROOTSTREAM_SCENE_DETECTOR_H
#define ROOTSTREAM_SCENE_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Number of histogram bins (covers full [0,255] luma range) */
#define SCENE_HIST_BINS 64

/** Result of a scene-change test */
typedef struct {
    bool     scene_changed;      /**< True if a cut/transition was detected */
    double   histogram_diff;     /**< Normalised histogram difference [0,1] */
    uint64_t frame_number;       /**< Frame index (monotonic, from detector) */
} scene_result_t;

/** Configuration for the scene detector */
typedef struct {
    double threshold;            /**< Histogram diff to declare a change [0,1] */
    int    warmup_frames;        /**< Frames to skip at startup (default: 2) */
} scene_config_t;

/** Opaque scene detector state */
typedef struct scene_detector_s scene_detector_t;

/**
 * scene_detector_create — allocate detector with given config
 *
 * @param config  Configuration; NULL uses defaults (threshold=0.35)
 * @return        Non-NULL handle, or NULL on OOM
 */
scene_detector_t *scene_detector_create(const scene_config_t *config);

/**
 * scene_detector_destroy — free detector state
 *
 * @param det  Detector to destroy
 */
void scene_detector_destroy(scene_detector_t *det);

/**
 * scene_detector_push — submit a luma frame and get a scene-change decision
 *
 * @param det     Detector state
 * @param luma    Pointer to luma (Y) plane, row-major
 * @param width   Frame width in pixels
 * @param height  Frame height in pixels
 * @param stride  Row stride in bytes (>= width)
 * @return        Scene change result for this frame
 */
scene_result_t scene_detector_push(scene_detector_t *det,
                                    const uint8_t    *luma,
                                    int               width,
                                    int               height,
                                    int               stride);

/**
 * scene_detector_reset — discard history, restart warmup
 *
 * Call after a manual IDR or stream restart.
 *
 * @param det  Detector state
 */
void scene_detector_reset(scene_detector_t *det);

/**
 * scene_detector_frame_count — return total frames pushed so far
 *
 * @param det  Detector state
 * @return     Frame count
 */
uint64_t scene_detector_frame_count(const scene_detector_t *det);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SCENE_DETECTOR_H */
