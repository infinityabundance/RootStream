/*
 * phash_dedup.h — Near-duplicate frame detector using pHash
 *
 * Wraps the pHash index to provide a streaming near-duplicate detector.
 * Call `phash_dedup_push()` with each new frame's hash; it returns true
 * if the frame is a near-duplicate of any previously indexed frame.
 *
 * Suitable for dropping redundant keyframes from recordings or skipping
 * unchanged screen regions in remote desktop streaming.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_PHASH_DEDUP_H
#define ROOTSTREAM_PHASH_DEDUP_H

#include <stdbool.h>
#include <stddef.h>

#include "phash.h"
#include "phash_index.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque deduplification context */
typedef struct phash_dedup_s phash_dedup_t;

/**
 * phash_dedup_create — allocate dedup context
 *
 * @param max_dist  Hamming distance threshold for "duplicate" (typically 5–10)
 * @return          Non-NULL handle, or NULL on OOM
 */
phash_dedup_t *phash_dedup_create(int max_dist);

/**
 * phash_dedup_destroy — free context
 *
 * @param d  Context to destroy
 */
void phash_dedup_destroy(phash_dedup_t *d);

/**
 * phash_dedup_push — check and register a new frame hash
 *
 * If the hash is a near-duplicate of an already-indexed frame, returns
 * true and does NOT add the duplicate to the index.
 * Otherwise, inserts the hash and returns false.
 *
 * @param d           Dedup context
 * @param hash        Frame perceptual hash
 * @param frame_id    Caller-assigned frame identifier
 * @param out_match   If non-NULL and frame is a duplicate: set to
 *                    the id of the matching existing frame
 * @return            true  = duplicate (frame should be skipped/dropped)
 *                    false = unique frame (has been indexed)
 */
bool phash_dedup_push(phash_dedup_t *d, uint64_t hash, uint64_t frame_id, uint64_t *out_match);

/**
 * phash_dedup_reset — clear all indexed hashes
 *
 * @param d  Dedup context
 */
void phash_dedup_reset(phash_dedup_t *d);

/**
 * phash_dedup_indexed_count — number of unique frames indexed
 *
 * @param d  Dedup context
 * @return   Count
 */
size_t phash_dedup_indexed_count(const phash_dedup_t *d);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PHASH_DEDUP_H */
