/*
 * sr_stats.h — Signal Router statistics
 *
 * Tracks total signals routed (delivered to ≥1 route), filtered
 * (matched but rejected by filter_fn), and dropped (no route matched).
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_SR_STATS_H
#define ROOTSTREAM_SR_STATS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Signal router statistics snapshot */
typedef struct {
    uint64_t routed;   /**< Signals delivered to ≥1 route */
    uint64_t filtered; /**< Signals matched but blocked by filter_fn */
    uint64_t dropped;  /**< Signals with 0 matching routes */
} sr_stats_snapshot_t;

/** Opaque signal router stats context */
typedef struct sr_stats_s sr_stats_t;

sr_stats_t *sr_stats_create(void);
void sr_stats_destroy(sr_stats_t *st);

int sr_stats_record_route(sr_stats_t *st, int delivered, int filtered_n);
int sr_stats_snapshot(const sr_stats_t *st, sr_stats_snapshot_t *out);
void sr_stats_reset(sr_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SR_STATS_H */
