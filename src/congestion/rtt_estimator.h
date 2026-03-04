/*
 * rtt_estimator.h — RTT / SRTT / RTTVAR estimator (RFC 6298)
 *
 * Implements the smoothed RTT estimator as defined in RFC 6298 §2:
 *
 *   SRTT    ← (1 − α) · SRTT + α · R       α = 1/8
 *   RTTVAR  ← (1 − β) · RTTVAR + β · |SRTT − R|  β = 1/4
 *   RTO     = SRTT + max(G, K · RTTVAR)     K = 4, G = clock granularity
 *
 * Caller supplies RTT samples in microseconds.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_RTT_ESTIMATOR_H
#define ROOTSTREAM_RTT_ESTIMATOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** RFC 6298 clock granularity in µs (1 ms) */
#define RTT_CLOCK_GRANULARITY_US  1000ULL

/** RFC 6298 K constant */
#define RTT_K  4

/** RTT statistics snapshot */
typedef struct {
    double   srtt_us;        /**< Smoothed RTT (µs) */
    double   rttvar_us;      /**< RTT variance (µs) */
    double   rto_us;         /**< Retransmit timeout (µs) */
    double   min_rtt_us;     /**< Minimum observed RTT (µs) */
    double   max_rtt_us;     /**< Maximum observed RTT (µs) */
    uint64_t sample_count;   /**< Total RTT samples processed */
} rtt_snapshot_t;

/** Opaque RTT estimator */
typedef struct rtt_estimator_s rtt_estimator_t;

/**
 * rtt_estimator_create — allocate estimator
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
rtt_estimator_t *rtt_estimator_create(void);

/**
 * rtt_estimator_destroy — free estimator
 *
 * @param e  Estimator to destroy
 */
void rtt_estimator_destroy(rtt_estimator_t *e);

/**
 * rtt_estimator_update — feed a new RTT sample
 *
 * On the first sample the RFC 6298 §2.2 initialization is applied:
 *   SRTT   = R, RTTVAR = R/2
 *
 * @param e       Estimator
 * @param rtt_us  Observed RTT in µs
 * @return        0 on success, -1 on NULL / invalid sample
 */
int rtt_estimator_update(rtt_estimator_t *e, uint64_t rtt_us);

/**
 * rtt_estimator_snapshot — copy current estimates
 *
 * @param e    Estimator
 * @param out  Output snapshot
 * @return     0 on success, -1 on NULL
 */
int rtt_estimator_snapshot(const rtt_estimator_t *e, rtt_snapshot_t *out);

/**
 * rtt_estimator_reset — clear all samples
 *
 * @param e  Estimator
 */
void rtt_estimator_reset(rtt_estimator_t *e);

/**
 * rtt_estimator_has_samples — return true if at least one sample
 *
 * @param e  Estimator
 * @return   true if samples available
 */
bool rtt_estimator_has_samples(const rtt_estimator_t *e);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_RTT_ESTIMATOR_H */
