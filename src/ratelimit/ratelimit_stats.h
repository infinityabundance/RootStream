/*
 * ratelimit_stats.h — Token bucket / rate limiter statistics
 *
 * Tracks per-registry totals: packets allowed, packets throttled, and
 * total bytes consumed for capacity planning.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_RATELIMIT_STATS_H
#define ROOTSTREAM_RATELIMIT_STATS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Rate limiter statistics snapshot */
typedef struct {
    uint64_t packets_allowed;   /**< Total packets that passed */
    uint64_t packets_throttled; /**< Total packets that were dropped */
    double   bytes_consumed;    /**< Total bytes that passed */
    double   throttle_rate;     /**< throttled / (allowed + throttled) */
} ratelimit_stats_snapshot_t;

/** Opaque stats context */
typedef struct ratelimit_stats_s ratelimit_stats_t;

/**
 * ratelimit_stats_create — allocate stats context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
ratelimit_stats_t *ratelimit_stats_create(void);

/**
 * ratelimit_stats_destroy — free context
 *
 * @param st  Context to destroy
 */
void ratelimit_stats_destroy(ratelimit_stats_t *st);

/**
 * ratelimit_stats_record — record one consume decision
 *
 * @param st       Stats context
 * @param allowed  true if the packet was allowed
 * @param bytes    Packet size in bytes
 * @return         0 on success, -1 on NULL
 */
int ratelimit_stats_record(ratelimit_stats_t *st, int allowed, double bytes);

/**
 * ratelimit_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int ratelimit_stats_snapshot(const ratelimit_stats_t    *st,
                               ratelimit_stats_snapshot_t *out);

/**
 * ratelimit_stats_reset — clear all accumulators
 *
 * @param st  Context
 */
void ratelimit_stats_reset(ratelimit_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_RATELIMIT_STATS_H */
