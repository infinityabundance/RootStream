/*
 * rate_limiter.c — Per-viewer rate limiter registry implementation
 */

#include "rate_limiter.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    uint64_t        viewer_id;
    token_bucket_t *bucket;
    bool            valid;
} rl_entry_t;

struct rate_limiter_s {
    rl_entry_t entries[RATE_LIMITER_MAX_VIEWERS];
    size_t     count;
    double     default_rate_bps;
    double     default_burst;
};

rate_limiter_t *rate_limiter_create(double default_rate_bps, double default_burst) {
    if (default_rate_bps <= 0.0 || default_burst <= 0.0) return NULL;
    rate_limiter_t *rl = calloc(1, sizeof(*rl));
    if (!rl) return NULL;
    rl->default_rate_bps = default_rate_bps;
    rl->default_burst    = default_burst;
    return rl;
}

void rate_limiter_destroy(rate_limiter_t *rl) {
    if (!rl) return;
    for (size_t i = 0; i < RATE_LIMITER_MAX_VIEWERS; i++) {
        if (rl->entries[i].valid)
            token_bucket_destroy(rl->entries[i].bucket);
    }
    free(rl);
}

static rl_entry_t *find_entry(rate_limiter_t *rl, uint64_t viewer_id) {
    for (size_t i = 0; i < RATE_LIMITER_MAX_VIEWERS; i++) {
        if (rl->entries[i].valid && rl->entries[i].viewer_id == viewer_id)
            return &rl->entries[i];
    }
    return NULL;
}

int rate_limiter_add_viewer(rate_limiter_t *rl, uint64_t viewer_id, uint64_t now_us) {
    if (!rl) return -1;
    if (find_entry(rl, viewer_id)) return 0; /* already exists */
    if (rl->count >= RATE_LIMITER_MAX_VIEWERS) return -1;

    for (size_t i = 0; i < RATE_LIMITER_MAX_VIEWERS; i++) {
        if (!rl->entries[i].valid) {
            rl->entries[i].bucket = token_bucket_create(
                rl->default_rate_bps, rl->default_burst, now_us);
            if (!rl->entries[i].bucket) return -1;
            rl->entries[i].viewer_id = viewer_id;
            rl->entries[i].valid     = true;
            rl->count++;
            return 0;
        }
    }
    return -1;
}

int rate_limiter_remove_viewer(rate_limiter_t *rl, uint64_t viewer_id) {
    if (!rl) return -1;
    for (size_t i = 0; i < RATE_LIMITER_MAX_VIEWERS; i++) {
        if (rl->entries[i].valid && rl->entries[i].viewer_id == viewer_id) {
            token_bucket_destroy(rl->entries[i].bucket);
            rl->entries[i].valid  = false;
            rl->entries[i].bucket = NULL;
            rl->count--;
            return 0;
        }
    }
    return -1;
}

bool rate_limiter_consume(rate_limiter_t *rl, uint64_t viewer_id,
                            double bytes, uint64_t now_us) {
    if (!rl) return false;
    rl_entry_t *e = find_entry(rl, viewer_id);
    if (!e) return false;
    return token_bucket_consume(e->bucket, bytes, now_us);
}

size_t rate_limiter_viewer_count(const rate_limiter_t *rl) {
    return rl ? rl->count : 0;
}

bool rate_limiter_has_viewer(const rate_limiter_t *rl, uint64_t viewer_id) {
    if (!rl) return false;
    for (size_t i = 0; i < RATE_LIMITER_MAX_VIEWERS; i++) {
        if (rl->entries[i].valid && rl->entries[i].viewer_id == viewer_id)
            return true;
    }
    return false;
}
