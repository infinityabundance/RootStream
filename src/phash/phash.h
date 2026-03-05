/*
 * phash.h — DCT-based 64-bit perceptual hash (pHash) for video frames
 *
 * Computes a 64-bit fingerprint of an 8-bit luma plane by:
 *   1. Resize to 32×32 pixels (bilinear)
 *   2. Apply a simplified 2D DCT-II
 *   3. Extract the top-left 8×8 = 64 DCT coefficients (excl. DC)
 *   4. Compare each to the mean; set bit 1 if above mean, 0 otherwise
 *
 * Hamming distance between two pHashes approximates visual similarity:
 *   0–5   : near-identical
 *   6–10  : slight difference
 *   11–20 : notable difference
 *   > 20  : different scene
 *
 * Thread-safety: all functions are stateless and thread-safe.
 */

#ifndef ROOTSTREAM_PHASH_H
#define ROOTSTREAM_PHASH_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Size of the internal work grid */
#define PHASH_WORK_SIZE   32
/** Size of the feature region extracted from the DCT */
#define PHASH_FEAT_SIZE    8
/** Resulting hash width in bits */
#define PHASH_BITS        (PHASH_FEAT_SIZE * PHASH_FEAT_SIZE)

/**
 * phash_compute — compute 64-bit perceptual hash of an 8-bit luma plane
 *
 * @param luma    Pointer to width*height 8-bit luma samples (row-major)
 * @param width   Frame width in pixels
 * @param height  Frame height in pixels
 * @param stride  Bytes per row (>= width)
 * @param out     64-bit pHash output
 * @return        0 on success, -1 on NULL/invalid args
 */
int phash_compute(const uint8_t *luma,
                   int            width,
                   int            height,
                   int            stride,
                   uint64_t      *out);

/**
 * phash_hamming — compute Hamming distance between two pHashes
 *
 * @param a  First hash
 * @param b  Second hash
 * @return   Number of differing bits [0, 64]
 */
int phash_hamming(uint64_t a, uint64_t b);

/**
 * phash_similar — return true if two hashes are perceptually similar
 *
 * Uses a threshold of <= @max_dist Hamming bits.
 *
 * @param a         First hash
 * @param b         Second hash
 * @param max_dist  Maximum Hamming distance to consider similar
 * @return          true if similar
 */
bool phash_similar(uint64_t a, uint64_t b, int max_dist);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PHASH_H */
