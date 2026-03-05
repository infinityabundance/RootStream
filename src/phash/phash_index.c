/*
 * phash_index.c — pHash index implementation (linear scan)
 */

#include "phash_index.h"

#include <stdlib.h>
#include <string.h>

struct phash_index_s {
    phash_entry_t entries[PHASH_INDEX_MAX_ENTRIES];
    size_t        count;
};

phash_index_t *phash_index_create(void) {
    phash_index_t *idx = calloc(1, sizeof(*idx));
    return idx;
}

void phash_index_destroy(phash_index_t *idx) {
    free(idx);
}

size_t phash_index_count(const phash_index_t *idx) {
    return idx ? idx->count : 0;
}

int phash_index_insert(phash_index_t *idx, uint64_t hash, uint64_t id) {
    if (!idx || idx->count >= PHASH_INDEX_MAX_ENTRIES) return -1;
    /* Find a free slot */
    for (size_t i = 0; i < PHASH_INDEX_MAX_ENTRIES; i++) {
        if (!idx->entries[i].valid) {
            idx->entries[i].hash  = hash;
            idx->entries[i].id    = id;
            idx->entries[i].valid = true;
            idx->count++;
            return 0;
        }
    }
    return -1;
}

int phash_index_remove(phash_index_t *idx, uint64_t id) {
    if (!idx) return -1;
    for (size_t i = 0; i < PHASH_INDEX_MAX_ENTRIES; i++) {
        if (idx->entries[i].valid && idx->entries[i].id == id) {
            idx->entries[i].valid = false;
            idx->count--;
            return 0;
        }
    }
    return -1;
}

int phash_index_nearest(const phash_index_t *idx,
                          uint64_t             query,
                          uint64_t            *out_id,
                          int                 *out_dist) {
    if (!idx || !out_id || !out_dist || idx->count == 0) return -1;

    int      best_dist = 65;
    uint64_t best_id   = 0;

    for (size_t i = 0; i < PHASH_INDEX_MAX_ENTRIES; i++) {
        if (!idx->entries[i].valid) continue;
        int d = phash_hamming(query, idx->entries[i].hash);
        if (d < best_dist) {
            best_dist = d;
            best_id   = idx->entries[i].id;
        }
    }

    if (best_dist == 65) return -1;
    *out_id   = best_id;
    *out_dist = best_dist;
    return 0;
}

size_t phash_index_range_query(const phash_index_t *idx,
                                 uint64_t             query,
                                 int                  max_dist,
                                 phash_entry_t       *out,
                                 size_t               out_max) {
    if (!idx || !out || out_max == 0) return 0;
    size_t found = 0;
    for (size_t i = 0; i < PHASH_INDEX_MAX_ENTRIES && found < out_max; i++) {
        if (!idx->entries[i].valid) continue;
        if (phash_hamming(query, idx->entries[i].hash) <= max_dist)
            out[found++] = idx->entries[i];
    }
    return found;
}
