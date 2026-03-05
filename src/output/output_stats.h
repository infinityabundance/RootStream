/*
 * output_stats.h — Output target registry statistics
 *
 * Aggregates bytes_sent, connect_count, error_count across all
 * targets and tracks the number of currently active targets.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_OUTPUT_STATS_H
#define ROOTSTREAM_OUTPUT_STATS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Output stats snapshot */
typedef struct {
    uint64_t bytes_sent;     /**< Total bytes transmitted across all targets */
    uint32_t connect_count;  /**< Successful connect events */
    uint32_t error_count;    /**< Failed connect / error events */
    int      active_count;   /**< Currently active (OT_ACTIVE) targets */
} output_stats_snapshot_t;

/** Opaque output stats context */
typedef struct output_stats_s output_stats_t;

/**
 * output_stats_create — allocate context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
output_stats_t *output_stats_create(void);

/**
 * output_stats_destroy — free context
 */
void output_stats_destroy(output_stats_t *st);

/**
 * output_stats_record_bytes — add bytes transmitted
 *
 * @param st     Context
 * @param bytes  Bytes sent this call
 * @return       0 on success, -1 on NULL
 */
int output_stats_record_bytes(output_stats_t *st, uint64_t bytes);

/**
 * output_stats_record_connect — record a successful connection
 *
 * @param st  Context
 * @return    0 on success, -1 on NULL
 */
int output_stats_record_connect(output_stats_t *st);

/**
 * output_stats_record_error — record a connect/error event
 *
 * @param st  Context
 * @return    0 on success, -1 on NULL
 */
int output_stats_record_error(output_stats_t *st);

/**
 * output_stats_set_active — update current active target count
 *
 * @param st    Context
 * @param count Active count
 * @return      0 on success, -1 on NULL
 */
int output_stats_set_active(output_stats_t *st, int count);

/**
 * output_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int output_stats_snapshot(const output_stats_t *st, output_stats_snapshot_t *out);

/**
 * output_stats_reset — clear all statistics
 */
void output_stats_reset(output_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_OUTPUT_STATS_H */
