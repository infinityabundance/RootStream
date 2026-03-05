/*
 * probe_estimator.h — One-way delay and bandwidth estimator
 *
 * Consumes (send_ts_us, recv_ts_us, size_bytes) observations from
 * received probe packets and estimates:
 *   - one-way delay (OWD) using a EWMA smoother
 *   - available bandwidth by dividing burst payload over burst duration
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_PROBE_ESTIMATOR_H
#define ROOTSTREAM_PROBE_ESTIMATOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** EWMA smoothing factor for OWD (α = 1/8) */
#define PROBE_OWD_ALPHA  0.125

/** Bandwidth estimate snapshot */
typedef struct {
    double   owd_us;           /**< Smoothed one-way delay (µs) */
    double   owd_min_us;       /**< Minimum observed OWD (µs) */
    double   owd_max_us;       /**< Maximum observed OWD (µs) */
    double   bw_bps;           /**< Estimated available bandwidth (bits/s) */
    uint64_t sample_count;     /**< Total observations fed */
} probe_estimate_t;

/** Opaque estimator */
typedef struct probe_estimator_s probe_estimator_t;

/**
 * probe_estimator_create — allocate estimator
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
probe_estimator_t *probe_estimator_create(void);

/**
 * probe_estimator_destroy — free estimator
 *
 * @param pe  Estimator to destroy
 */
void probe_estimator_destroy(probe_estimator_t *pe);

/**
 * probe_estimator_observe — feed one received probe packet
 *
 * @param pe           Estimator
 * @param send_ts_us   Sender timestamp (from probe_packet_t.send_ts_us)
 * @param recv_ts_us   Local receive timestamp (µs)
 * @param size_bytes   Payload size (use PROBE_PKT_SIZE for plain probes)
 * @return             0 on success, -1 on error
 */
int probe_estimator_observe(probe_estimator_t *pe,
                              uint64_t            send_ts_us,
                              uint64_t            recv_ts_us,
                              uint32_t            size_bytes);

/**
 * probe_estimator_snapshot — copy current estimates
 *
 * @param pe   Estimator
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int probe_estimator_snapshot(const probe_estimator_t *pe, probe_estimate_t *out);

/**
 * probe_estimator_reset — clear all observations
 *
 * @param pe  Estimator
 */
void probe_estimator_reset(probe_estimator_t *pe);

/**
 * probe_estimator_has_samples — return true if at least one observation
 *
 * @param pe  Estimator
 * @return    true if samples available
 */
bool probe_estimator_has_samples(const probe_estimator_t *pe);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PROBE_ESTIMATOR_H */
