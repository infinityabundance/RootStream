/*
 * cs_stats.h — Clock sync statistics
 *
 * Accumulates per-sample RTT and offset observations to produce
 * min/average/max summaries and a convergence flag.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_CS_STATS_H
#define ROOTSTREAM_CS_STATS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CS_CONVERGENCE_SAMPLES 8 /**< Samples required before convergence */

/** Clock sync statistics snapshot */
typedef struct {
    uint64_t sample_count; /**< Total samples observed */
    int64_t min_offset_us; /**< Minimum clock offset seen */
    int64_t max_offset_us; /**< Maximum clock offset seen */
    double avg_offset_us;  /**< Running average offset */
    int64_t min_rtt_us;    /**< Minimum RTT seen */
    int64_t max_rtt_us;    /**< Maximum RTT seen */
    double avg_rtt_us;     /**< Running average RTT */
    bool converged;        /**< True after CS_CONVERGENCE_SAMPLES */
} cs_stats_snapshot_t;

/** Opaque clock sync stats context */
typedef struct cs_stats_s cs_stats_t;

/**
 * cs_stats_create — allocate stats context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
cs_stats_t *cs_stats_create(void);

/**
 * cs_stats_destroy — free context
 */
void cs_stats_destroy(cs_stats_t *st);

/**
 * cs_stats_record — record one (offset, rtt) observation
 *
 * @param st         Context
 * @param offset_us  Signed clock offset in µs
 * @param rtt_us     Round-trip delay in µs
 * @return           0 on success, -1 on NULL
 */
int cs_stats_record(cs_stats_t *st, int64_t offset_us, int64_t rtt_us);

/**
 * cs_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int cs_stats_snapshot(const cs_stats_t *st, cs_stats_snapshot_t *out);

/**
 * cs_stats_reset — clear all statistics
 */
void cs_stats_reset(cs_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CS_STATS_H */
