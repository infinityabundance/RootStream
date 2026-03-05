/*
 * kfr_stats.h — Keyframe request handler statistics
 *
 * Tracks requests sent, received, forwarded, and suppressed per handler.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_KFR_STATS_H
#define ROOTSTREAM_KFR_STATS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Keyframe stats snapshot */
typedef struct {
    uint64_t requests_received;   /**< Total requests submitted */
    uint64_t requests_forwarded;  /**< Requests forwarded to encoder */
    uint64_t requests_suppressed; /**< Requests suppressed (dedup / cooldown) */
    uint64_t urgent_requests;     /**< Requests with priority > 0 */
    double   suppression_rate;    /**< suppressed / received */
} kfr_stats_snapshot_t;

/** Opaque stats context */
typedef struct kfr_stats_s kfr_stats_t;

/**
 * kfr_stats_create — allocate stats context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
kfr_stats_t *kfr_stats_create(void);

/**
 * kfr_stats_destroy — free context
 *
 * @param st  Context to destroy
 */
void kfr_stats_destroy(kfr_stats_t *st);

/**
 * kfr_stats_record — record one request outcome
 *
 * @param st        Context
 * @param forwarded 1 if forwarded, 0 if suppressed
 * @param urgent    1 if request had priority > 0
 * @return          0 on success, -1 on NULL
 */
int kfr_stats_record(kfr_stats_t *st, int forwarded, int urgent);

/**
 * kfr_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int kfr_stats_snapshot(const kfr_stats_t *st, kfr_stats_snapshot_t *out);

/**
 * kfr_stats_reset — clear all statistics
 *
 * @param st  Context
 */
void kfr_stats_reset(kfr_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_KFR_STATS_H */
