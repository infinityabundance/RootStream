/*
 * phash_dedup.c — Near-duplicate frame detector implementation
 */

#include "phash_dedup.h"

#include <stdlib.h>

struct phash_dedup_s {
    phash_index_t *idx;
    int max_dist;
};

phash_dedup_t *phash_dedup_create(int max_dist) {
    phash_dedup_t *d = malloc(sizeof(*d));
    if (!d)
        return NULL;
    d->idx = phash_index_create();
    if (!d->idx) {
        free(d);
        return NULL;
    }
    d->max_dist = max_dist;
    return d;
}

void phash_dedup_destroy(phash_dedup_t *d) {
    if (!d)
        return;
    phash_index_destroy(d->idx);
    free(d);
}

void phash_dedup_reset(phash_dedup_t *d) {
    if (!d || !d->idx)
        return;
    phash_index_destroy(d->idx);
    d->idx = phash_index_create();
}

size_t phash_dedup_indexed_count(const phash_dedup_t *d) {
    return d ? phash_index_count(d->idx) : 0;
}

bool phash_dedup_push(phash_dedup_t *d, uint64_t hash, uint64_t frame_id, uint64_t *out_match) {
    if (!d || !d->idx)
        return false;

    uint64_t match_id = 0;
    int match_dist = 0;

    if (phash_index_nearest(d->idx, hash, &match_id, &match_dist) == 0 &&
        match_dist <= d->max_dist) {
        /* Duplicate */
        if (out_match)
            *out_match = match_id;
        return true;
    }

    /* Unique — index it */
    phash_index_insert(d->idx, hash, frame_id);
    return false;
}
