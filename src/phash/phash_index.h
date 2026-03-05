/*
 * phash_index.h — In-memory pHash index with Hamming distance lookup
 *
 * Stores a set of (hash, id) pairs and supports:
 *   - Insert a new fingerprint
 *   - Nearest-neighbour lookup (minimum Hamming distance)
 *   - Range query (all hashes within Hamming distance d)
 *   - Remove by id
 *
 * Implemented as a linear scan; suitable for up to ~50,000 entries.
 * For larger collections a VP-tree or BK-tree can replace this module.
 *
 * Thread-safety: NOT thread-safe.  Callers must synchronise.
 */

#ifndef ROOTSTREAM_PHASH_INDEX_H
#define ROOTSTREAM_PHASH_INDEX_H

#include "phash.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PHASH_INDEX_MAX_ENTRIES  65536

/** A single index entry */
typedef struct {
    uint64_t hash;
    uint64_t id;
    bool     valid;
} phash_entry_t;

/** Opaque pHash index handle */
typedef struct phash_index_s phash_index_t;

/**
 * phash_index_create — allocate empty index
 *
 * @return  Non-NULL handle, or NULL on OOM
 */
phash_index_t *phash_index_create(void);

/**
 * phash_index_destroy — free index
 *
 * @param idx  Index to destroy
 */
void phash_index_destroy(phash_index_t *idx);

/**
 * phash_index_insert — add a hash with associated @id
 *
 * @param idx   Index
 * @param hash  Perceptual hash
 * @param id    Caller-assigned identifier (e.g. frame number)
 * @return      0 on success, -1 if full or null args
 */
int phash_index_insert(phash_index_t *idx, uint64_t hash, uint64_t id);

/**
 * phash_index_remove — remove entry with @id
 *
 * @param idx  Index
 * @param id   Identifier to remove
 * @return     0 on success, -1 if not found
 */
int phash_index_remove(phash_index_t *idx, uint64_t id);

/**
 * phash_index_nearest — find entry with minimum Hamming distance to @query
 *
 * @param idx       Index
 * @param query     Query hash
 * @param out_id    Nearest entry id (if found)
 * @param out_dist  Hamming distance to nearest entry
 * @return          0 if a match found, -1 if index empty
 */
int phash_index_nearest(const phash_index_t *idx,
                          uint64_t             query,
                          uint64_t            *out_id,
                          int                 *out_dist);

/**
 * phash_index_range_query — find all entries within @max_dist of @query
 *
 * @param idx       Index
 * @param query     Query hash
 * @param max_dist  Maximum Hamming distance
 * @param out       Output array of matching entries
 * @param out_max   Capacity of @out
 * @return          Number of matches (may be < actual count if out_max too small)
 */
size_t phash_index_range_query(const phash_index_t *idx,
                                 uint64_t             query,
                                 int                  max_dist,
                                 phash_entry_t       *out,
                                 size_t               out_max);

/**
 * phash_index_count — number of valid entries in index
 *
 * @param idx  Index
 * @return     Count
 */
size_t phash_index_count(const phash_index_t *idx);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PHASH_INDEX_H */
