/*
 * jitter_stats.h — Jitter measurement statistics
 *
 * Computes inter-arrival jitter (RFC 3550 §A.8), end-to-end delay,
 * and packet loss/late metrics for a jitter buffer session.
 *
 * RFC 3550 jitter estimator:
 *   J(i) = J(i-1) + (|D(i-1,i)| - J(i-1)) / 16
 *   D(i-1,i) = |receive_time_i - send_time_i| - |receive_time_{i-1} - send_time_{i-1}|
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_JITTER_STATS_H
#define ROOTSTREAM_JITTER_STATS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Jitter statistics snapshot */
typedef struct {
    uint64_t packets_received;   /**< Total packets received */
    uint64_t packets_late;       /**< Packets that arrived past playout deadline */
    uint64_t packets_dropped;    /**< Packets dropped (buffer full) */
    double   jitter_us;          /**< RFC 3550 inter-arrival jitter estimate (µs) */
    double   avg_delay_us;       /**< Running average end-to-end delay (µs) */
    double   min_delay_us;       /**< Minimum observed delay */
    double   max_delay_us;       /**< Maximum observed delay */
} jitter_stats_snapshot_t;

/** Opaque jitter stats context */
typedef struct jitter_stats_s jitter_stats_t;

/**
 * jitter_stats_create — allocate stats context
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
jitter_stats_t *jitter_stats_create(void);

/**
 * jitter_stats_destroy — free context
 *
 * @param st  Context to destroy
 */
void jitter_stats_destroy(jitter_stats_t *st);

/**
 * jitter_stats_record_arrival — record a packet arrival
 *
 * @param st           Stats context
 * @param send_us      Sender timestamp (µs)
 * @param recv_us      Receiver timestamp (µs, local clock)
 * @param is_late      1 if packet arrived after playout deadline
 * @param was_dropped  1 if a packet was dropped to accommodate this one
 * @return             0 on success, -1 on NULL args
 */
int jitter_stats_record_arrival(jitter_stats_t *st,
                                  uint64_t        send_us,
                                  uint64_t        recv_us,
                                  int             is_late,
                                  int             was_dropped);

/**
 * jitter_stats_snapshot — copy current statistics
 *
 * @param st   Stats context
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL args
 */
int jitter_stats_snapshot(const jitter_stats_t    *st,
                            jitter_stats_snapshot_t *out);

/**
 * jitter_stats_reset — clear all statistics
 *
 * @param st  Stats context
 */
void jitter_stats_reset(jitter_stats_t *st);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_JITTER_STATS_H */
