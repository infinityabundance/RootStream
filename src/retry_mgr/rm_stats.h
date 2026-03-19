/*
 * rm_stats.h — Retry Manager statistics
 *
 * Tracks aggregate retry activity: total attempt counts, successful
 * completions, requests that exhausted their retry budget, and the
 * highest number of attempts ever observed for a single request.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_RM_STATS_H
#define ROOTSTREAM_RM_STATS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Retry manager statistics snapshot */
typedef struct {
    uint64_t total_attempts;    /**< Total individual attempt firings */
    uint32_t total_succeeded;   /**< Requests removed as succeeded */
    uint32_t total_expired;     /**< Requests removed after exhausting budget */
    uint32_t max_attempts_seen; /**< Max attempts for any single request */
} rm_stats_snapshot_t;

/** Opaque retry stats context */
typedef struct rm_stats_s rm_stats_t;

/**
 * rm_stats_create — allocate context
 *
 * @return Non-NULL handle, or NULL on OOM
 */
rm_stats_t *rm_stats_create(void);

/**
 * rm_stats_destroy — free context
 */
void rm_stats_destroy(rm_stats_t *st);

/**
 * rm_stats_record_attempt — record one attempt for a request
 *
 * @param st            Context
 * @param attempt_count Cumulative attempt count for this request
 * @return              0 on success, -1 on NULL
 */
int rm_stats_record_attempt(rm_stats_t *st, uint32_t attempt_count);

/**
 * rm_stats_record_success — record a successful request completion
 *
 * @param st  Context
 * @return    0 on success, -1 on NULL
 */
int rm_stats_record_success(rm_stats_t *st);

/**
 * rm_stats_record_expire — record a request that exhausted its budget
 *
 * @param st  Context
 * @return    0 on success, -1 on NULL
 */
int rm_stats_record_expire(rm_stats_t *st);

/**
 * rm_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int rm_stats_snapshot(const rm_stats_t *st, rm_stats_snapshot_t *out);

/**
 * rm_stats_reset — clear all statistics
 */
void rm_stats_reset(rm_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_RM_STATS_H */
