/*
 * ladder_rung.h — Single ABR bitrate ladder rung
 *
 * Represents one rendition in an adaptive-bitrate (ABR) ladder:
 * a target bitrate, output resolution, and frame rate.  Rungs are
 * ordered ascending by bitrate; the highest-fitting rung is selected
 * for the current network conditions.
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_LADDER_RUNG_H
#define ROOTSTREAM_LADDER_RUNG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Single ABR ladder rung */
typedef struct {
    uint32_t bitrate_bps;   /**< Target video bitrate (bits/second) */
    uint16_t width;         /**< Output width  (pixels) */
    uint16_t height;        /**< Output height (pixels) */
    float    fps;           /**< Output frame rate */
} ladder_rung_t;

/**
 * lr_init — initialise a rung
 *
 * @param r         Rung to initialise
 * @param bps       Target bitrate (bits/second, must be > 0)
 * @param width     Output width  (must be > 0)
 * @param height    Output height (must be > 0)
 * @param fps       Frame rate (must be > 0)
 * @return          0 on success, -1 on NULL or invalid
 */
int lr_init(ladder_rung_t *r,
              uint32_t bps, uint16_t width, uint16_t height, float fps);

/**
 * lr_compare — compare two rungs by bitrate (ascending)
 *
 * Suitable as a qsort comparator.
 *
 * @return  < 0 if a < b, 0 if equal, > 0 if a > b
 */
int lr_compare(const void *a, const void *b);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_LADDER_RUNG_H */
