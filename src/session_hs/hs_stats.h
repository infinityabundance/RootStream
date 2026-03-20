/*
 * hs_stats.h — Session handshake event counters and round-trip latency
 *
 * Tracks handshake attempt/success/failure counts and the round-trip
 * time (RTT) from the first HELLO to the READY state, measured in
 * microseconds using caller-supplied timestamps.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_HS_STATS_H
#define ROOTSTREAM_HS_STATS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Handshake statistics snapshot */
typedef struct {
    uint64_t attempts;  /**< Total handshake attempts started */
    uint64_t successes; /**< Handshakes reaching READY state */
    uint64_t failures;  /**< Handshakes ending in ERROR */
    uint64_t timeouts;  /**< Handshakes aborted for timeout */
    double avg_rtt_us;  /**< Mean HELLO→READY latency (µs) */
    double min_rtt_us;  /**< Minimum HELLO→READY latency (µs) */
    double max_rtt_us;  /**< Maximum HELLO→READY latency (µs) */
} hs_stats_snapshot_t;

/** Opaque handshake stats context */
typedef struct hs_stats_s hs_stats_t;

/**
 * hs_stats_create — allocate stats context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
hs_stats_t *hs_stats_create(void);

/**
 * hs_stats_destroy — free context
 *
 * @param st  Context to destroy
 */
void hs_stats_destroy(hs_stats_t *st);

/**
 * hs_stats_begin — record start of a new handshake attempt
 *
 * @param st      Context
 * @param now_us  Current time in µs
 * @return        0 on success, -1 on NULL
 */
int hs_stats_begin(hs_stats_t *st, uint64_t now_us);

/**
 * hs_stats_complete — record successful handshake completion (READY)
 *
 * @param st      Context
 * @param now_us  Current time in µs
 * @return        0 on success, -1 on NULL
 */
int hs_stats_complete(hs_stats_t *st, uint64_t now_us);

/**
 * hs_stats_fail — record handshake failure
 *
 * @param st  Context
 * @return    0 on success, -1 on NULL
 */
int hs_stats_fail(hs_stats_t *st);

/**
 * hs_stats_timeout — record handshake timeout
 *
 * @param st  Context
 * @return    0 on success, -1 on NULL
 */
int hs_stats_timeout(hs_stats_t *st);

/**
 * hs_stats_snapshot — copy current statistics
 *
 * @param st   Context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int hs_stats_snapshot(const hs_stats_t *st, hs_stats_snapshot_t *out);

/**
 * hs_stats_reset — clear all statistics
 *
 * @param st  Context
 */
void hs_stats_reset(hs_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_HS_STATS_H */
