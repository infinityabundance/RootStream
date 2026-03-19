/*
 * frc_stats.h — Frame rate controller statistics
 *
 * Tracks the actual delivered frame rate, number of dropped frames,
 * and number of duplicated frames over the session lifetime.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_FRC_STATS_H
#define ROOTSTREAM_FRC_STATS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** FRC statistics snapshot */
typedef struct {
    uint64_t frames_presented;  /**< Frames sent to the encoder/network */
    uint64_t frames_dropped;    /**< Frames discarded (encoder too fast) */
    uint64_t frames_duplicated; /**< Frames repeated (encoder too slow) */
    double actual_fps;          /**< Smoothed actual output frame rate */
} frc_stats_snapshot_t;

/** Opaque FRC stats */
typedef struct frc_stats_s frc_stats_t;

/**
 * frc_stats_create — allocate stats context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
frc_stats_t *frc_stats_create(void);

/**
 * frc_stats_destroy — free context
 *
 * @param st  Context to destroy
 */
void frc_stats_destroy(frc_stats_t *st);

/**
 * frc_stats_record — record one pacer tick outcome
 *
 * @param st         Stats context
 * @param presented  1 if frame was presented
 * @param dropped    1 if frame was dropped
 * @param duplicated 1 if frame was duplicated
 * @param now_ns     Current monotonic time in nanoseconds (for fps calc)
 * @return           0 on success, -1 on NULL
 */
int frc_stats_record(frc_stats_t *st, int presented, int dropped, int duplicated, uint64_t now_ns);

/**
 * frc_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int frc_stats_snapshot(const frc_stats_t *st, frc_stats_snapshot_t *out);

/**
 * frc_stats_reset — clear all statistics
 *
 * @param st  Context
 */
void frc_stats_reset(frc_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_FRC_STATS_H */
