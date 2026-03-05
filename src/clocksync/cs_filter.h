/*
 * cs_filter.h — 8-sample median filter for clock sync offset/RTT
 *
 * Accumulates up to CS_FILTER_SIZE NTP-style round-trip samples and
 * produces a median clock offset and median RTT.  Median filtering
 * rejects outliers caused by asymmetric queuing delays.
 *
 * Once CS_FILTER_SIZE samples are collected the filter is considered
 * "converged" and subsequent calls replace the oldest sample (sliding
 * window).
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_CS_FILTER_H
#define ROOTSTREAM_CS_FILTER_H

#include "cs_sample.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CS_FILTER_SIZE  8   /**< Sliding window size */

/** Filter output */
typedef struct {
    int64_t offset_us;  /**< Median clock offset (µs, signed) */
    int64_t rtt_us;     /**< Median RTT (µs) */
    bool    converged;  /**< True once CS_FILTER_SIZE samples collected */
    int     count;      /**< Samples collected so far (capped at size) */
} cs_filter_out_t;

/** Opaque clock sync filter */
typedef struct cs_filter_s cs_filter_t;

/**
 * cs_filter_create — allocate filter
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
cs_filter_t *cs_filter_create(void);

/**
 * cs_filter_destroy — free filter
 */
void cs_filter_destroy(cs_filter_t *f);

/**
 * cs_filter_push — add a sample and update medians
 *
 * @param f    Filter
 * @param s    Sample to add
 * @param out  Output median estimates (updated in-place)
 * @return     0 on success, -1 on NULL
 */
int cs_filter_push(cs_filter_t *f, const cs_sample_t *s, cs_filter_out_t *out);

/**
 * cs_filter_reset — clear all samples
 *
 * @param f  Filter
 */
void cs_filter_reset(cs_filter_t *f);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CS_FILTER_H */
