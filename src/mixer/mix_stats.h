/*
 * mix_stats.h — Multi-stream mixer statistics
 *
 * Tracks the number of sources that were active (non-muted, non-zero
 * weight) and muted during a mix call, cumulative buffer underruns
 * (when mix() is called with src_count == 0), and approximate mix
 * latency (caller-supplied µs timestamps).
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_MIX_STATS_H
#define ROOTSTREAM_MIX_STATS_H

#include <float.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Mix statistics snapshot */
typedef struct {
    uint64_t mix_calls;      /**< Total mix_engine_mix() calls recorded */
    uint64_t active_sources; /**< Cumulative active source-mix events */
    uint64_t muted_sources;  /**< Cumulative muted source-mix events */
    uint64_t underruns;      /**< Calls where no active sources contributed */
    double avg_latency_us;   /**< Average mix latency (µs) */
    double min_latency_us;   /**< Minimum mix latency (µs) */
    double max_latency_us;   /**< Maximum mix latency (µs) */
} mix_stats_snapshot_t;

/** Opaque mix stats context */
typedef struct mix_stats_s mix_stats_t;

/**
 * mix_stats_create — allocate stats context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
mix_stats_t *mix_stats_create(void);

/**
 * mix_stats_destroy — free context
 *
 * @param st  Context
 */
void mix_stats_destroy(mix_stats_t *st);

/**
 * mix_stats_record — record one mix call
 *
 * @param st              Context
 * @param active_count    Number of non-muted, non-zero-weight sources that
 *                        contributed to this mix call
 * @param muted_count     Number of muted sources that were skipped
 * @param latency_us      Mix latency in µs (0 if not measured)
 * @return                0 on success, -1 on NULL
 */
int mix_stats_record(mix_stats_t *st, int active_count, int muted_count, uint64_t latency_us);

/**
 * mix_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int mix_stats_snapshot(const mix_stats_t *st, mix_stats_snapshot_t *out);

/**
 * mix_stats_reset — clear all statistics
 *
 * @param st  Context
 */
void mix_stats_reset(mix_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_MIX_STATS_H */
