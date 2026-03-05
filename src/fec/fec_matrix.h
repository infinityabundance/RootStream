/*
 * fec_matrix.h — GF(2) XOR parity matrix builder for FEC
 *
 * Builds the parity (repair) blocks for a group of k source packets
 * using XOR operations over GF(2).  For each repair block r_i, the
 * parity vector p_i is a subset of the source indices selected by a
 * simple binary matrix row (identity-derived pattern).
 *
 * Repair block i is computed as:
 *   R_i = XOR of all source packets S_j where (j % (i+2)) == 0
 *
 * This guarantees each repair covers a distinct non-trivial subset
 * while keeping computation O(k × block_size).
 *
 * Thread-safety: stateless helpers — thread-safe.
 */

#ifndef ROOTSTREAM_FEC_MATRIX_H
#define ROOTSTREAM_FEC_MATRIX_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FEC_MAX_K        16   /**< Maximum source packets per group */
#define FEC_MAX_R        4    /**< Maximum repair packets per group */
#define FEC_MAX_PKT_SIZE 1472 /**< Maximum payload bytes per packet (UDP MTU) */

/**
 * fec_build_repair — compute repair[r] = XOR of selected sources
 *
 * @param sources     Array of k source payloads (each @pkt_size bytes)
 * @param k           Number of source packets (1..FEC_MAX_K)
 * @param repair_idx  Repair block index (0..FEC_MAX_R-1)
 * @param out         Output buffer (>= pkt_size bytes)
 * @param pkt_size    Payload size in bytes (<= FEC_MAX_PKT_SIZE)
 * @return            0 on success, -1 on error
 */
int fec_build_repair(const uint8_t *const *sources,
                       int                   k,
                       int                   repair_idx,
                       uint8_t              *out,
                       size_t                pkt_size);

/**
 * fec_repair_covers — return 1 if source[src_idx] contributes to repair[r]
 *
 * @param src_idx    Source index (0..k-1)
 * @param repair_idx Repair index (0..FEC_MAX_R-1)
 * @return           1 if source contributes, 0 otherwise
 */
int fec_repair_covers(int src_idx, int repair_idx);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_FEC_MATRIX_H */
