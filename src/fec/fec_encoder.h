/*
 * fec_encoder.h — FEC group encoder
 *
 * Encodes a group of k source packets into k+r packets where r ≤
 * FEC_MAX_R repair packets are appended.  Each repair packet is a
 * distinct XOR combination of the source packets (see fec_matrix.h).
 *
 * Group wire convention:
 *   Packets 0 .. k-1  : original source packets (pass-through)
 *   Packets k .. k+r-1: repair packets computed by fec_build_repair()
 *
 * Thread-safety: encoder is stateless — thread-safe.
 */

#ifndef ROOTSTREAM_FEC_ENCODER_H
#define ROOTSTREAM_FEC_ENCODER_H

#include "fec_matrix.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * fec_encode — encode a group of source packets into an output array
 *
 * @param sources   Array of k source payloads (each pkt_size bytes)
 * @param k         Number of source packets (1..FEC_MAX_K)
 * @param r         Number of repair packets to produce (0..FEC_MAX_R)
 * @param out       Output array of k+r buffers (each pkt_size bytes,
 *                  caller-allocated; first k are memcpy of sources)
 * @param pkt_size  Payload size in bytes (<= FEC_MAX_PKT_SIZE)
 * @return          0 on success, -1 on error
 */
int fec_encode(const uint8_t *const *sources,
                 int                   k,
                 int                   r,
                 uint8_t              **out,
                 size_t                pkt_size);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_FEC_ENCODER_H */
