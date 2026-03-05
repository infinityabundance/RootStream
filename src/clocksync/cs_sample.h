/*
 * cs_sample.h — NTP-style clock sync round-trip sample
 *
 * Captures the four NTP timestamps for one exchange:
 *   t0: client send time (local clock)
 *   t1: server receive time (remote clock)
 *   t2: server send time   (remote clock)
 *   t3: client receive time (local clock)
 *
 * From these, the round-trip delay d and clock offset θ are:
 *   d = (t3 - t0) - (t2 - t1)
 *   θ = ((t1 - t0) + (t2 - t3)) / 2
 *
 * All timestamps are in microseconds.
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_CS_SAMPLE_H
#define ROOTSTREAM_CS_SAMPLE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** NTP-style round-trip sample */
typedef struct {
    uint64_t t0;   /**< Client send      (µs, local clock) */
    uint64_t t1;   /**< Server receive   (µs, remote clock) */
    uint64_t t2;   /**< Server send      (µs, remote clock) */
    uint64_t t3;   /**< Client receive   (µs, local clock) */
} cs_sample_t;

/**
 * cs_sample_init — populate a sample
 *
 * @param s  Sample to fill
 * @param t0 Client send timestamp (µs)
 * @param t1 Server receive timestamp (µs)
 * @param t2 Server send timestamp (µs)
 * @param t3 Client receive timestamp (µs)
 * @return   0 on success, -1 on NULL
 */
int cs_sample_init(cs_sample_t *s,
                    uint64_t t0, uint64_t t1,
                    uint64_t t2, uint64_t t3);

/**
 * cs_sample_rtt_us — compute round-trip delay in µs
 *
 * @param s  Sample
 * @return   RTT in µs
 */
int64_t cs_sample_rtt_us(const cs_sample_t *s);

/**
 * cs_sample_offset_us — compute clock offset in µs
 *
 * Positive offset means the remote clock is ahead of the local clock.
 *
 * @param s  Sample
 * @return   Offset in µs (signed)
 */
int64_t cs_sample_offset_us(const cs_sample_t *s);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CS_SAMPLE_H */
