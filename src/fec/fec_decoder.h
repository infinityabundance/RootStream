/*
 * fec_decoder.h — FEC group decoder (XOR recovery)
 *
 * Given a received subset of k+r transmitted packets (where up to r
 * may be lost), recovers the original k source packets.
 *
 * Recovery is possible when the number of lost source packets ≤ r,
 * because each repair block is an XOR combination of the sources that
 * cover it.  Specifically:
 *
 *   repair[ri] = XOR of all source[j] where fec_repair_covers(j, ri).
 *
 * If source[j] is missing and exactly one repair covers it, the source
 * can be recovered by XOR-ing all other sources that the same repair
 * covers.  This decoder performs one pass of such single-missing
 * recovery for each repair block.
 *
 * Thread-safety: stateless function — thread-safe.
 */

#ifndef ROOTSTREAM_FEC_DECODER_H
#define ROOTSTREAM_FEC_DECODER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fec_matrix.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * fec_decode — attempt to recover lost source packets
 *
 * @param pkts       Array of k+r payload pointers.  Set pkts[i] to NULL
 *                   to indicate packet i was lost.
 * @param received   Array of k+r bools: received[i]=true if pkts[i] present
 * @param k          Number of source packets
 * @param r          Number of repair packets
 * @param pkt_size   Payload size in bytes
 * @param recovered  Output array of k allocated buffers; decoder writes
 *                   recovered data into recovered[j] for any lost source j.
 *                   Buffers for non-lost sources are left untouched.
 * @return           Number of source packets recovered (0..k), or -1 on error
 */
int fec_decode(const uint8_t *const *pkts, const bool *received, int k, int r, size_t pkt_size,
               uint8_t **recovered);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_FEC_DECODER_H */
