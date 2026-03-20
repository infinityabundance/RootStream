/*
 * fc_stats.h — Flow Controller statistics
 *
 * Tracks bytes actually sent, bytes dropped (consume refused), the
 * number of stall events (can_send returned false), and the number of
 * replenish calls received.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_FC_STATS_H
#define ROOTSTREAM_FC_STATS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Flow controller statistics snapshot */
typedef struct {
    uint64_t bytes_sent;      /**< Total bytes consumed successfully */
    uint64_t bytes_dropped;   /**< Total bytes from failed consume calls */
    uint64_t stalls;          /**< Times can_send returned false */
    uint64_t replenish_count; /**< Times replenish was called */
} fc_stats_snapshot_t;

/** Opaque flow stats context */
typedef struct fc_stats_s fc_stats_t;

fc_stats_t *fc_stats_create(void);
void fc_stats_destroy(fc_stats_t *st);

int fc_stats_record_send(fc_stats_t *st, uint32_t bytes);
int fc_stats_record_drop(fc_stats_t *st, uint32_t bytes);
int fc_stats_record_stall(fc_stats_t *st);
int fc_stats_record_replenish(fc_stats_t *st);

int fc_stats_snapshot(const fc_stats_t *st, fc_stats_snapshot_t *out);
void fc_stats_reset(fc_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_FC_STATS_H */
