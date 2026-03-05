/*
 * gop_stats.h — Adaptive GOP controller statistics
 *
 * Counts IDR frames by reason, natural (max-interval) IDRs, and
 * accumulates GOP length samples for an average-GOP-length estimate.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_GOP_STATS_H
#define ROOTSTREAM_GOP_STATS_H

#include "gop_controller.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** GOP statistics snapshot */
typedef struct {
    uint64_t total_frames;          /**< Total frames recorded */
    uint64_t idr_natural;           /**< IDRs from max-interval */
    uint64_t idr_scene_change;      /**< IDRs from scene-change detection */
    uint64_t idr_loss_recovery;     /**< IDRs from loss-recovery logic */
    uint64_t total_idrs;            /**< Sum of all IDR types */
    double   avg_gop_length;        /**< Average frames between IDRs */
} gop_stats_snapshot_t;

/** Opaque GOP stats context */
typedef struct gop_stats_s gop_stats_t;

/**
 * gop_stats_create — allocate stats context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
gop_stats_t *gop_stats_create(void);

/**
 * gop_stats_destroy — free context
 *
 * @param st  Context to destroy
 */
void gop_stats_destroy(gop_stats_t *st);

/**
 * gop_stats_record — record one frame decision
 *
 * @param st      Context
 * @param is_idr  1 if the frame is an IDR
 * @param reason  IDR reason (ignored when is_idr == 0)
 * @return        0 on success, -1 on NULL
 */
int gop_stats_record(gop_stats_t *st, int is_idr, gop_reason_t reason);

/**
 * gop_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int gop_stats_snapshot(const gop_stats_t *st, gop_stats_snapshot_t *out);

/**
 * gop_stats_reset — clear all statistics
 *
 * @param st  Context
 */
void gop_stats_reset(gop_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_GOP_STATS_H */
