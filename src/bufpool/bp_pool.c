/*
 * bp_pool.c — Fixed-size buffer pool implementation
 */

#include "bp_pool.h"

#include <stdlib.h>
#include <string.h>

struct bp_pool_s {
    bp_block_t blocks[BP_MAX_BLOCKS];
    void *backing; /* contiguous allocation for all blocks */
    int n_blocks;
    size_t block_size;
    int in_use;
    int peak;
};

bp_pool_t *bp_pool_create(int n_blocks, size_t block_size) {
    if (n_blocks < 1 || n_blocks > BP_MAX_BLOCKS || block_size == 0)
        return NULL;
    bp_pool_t *p = calloc(1, sizeof(*p));
    if (!p)
        return NULL;
    p->backing = calloc((size_t)n_blocks, block_size);
    if (!p->backing) {
        free(p);
        return NULL;
    }
    p->n_blocks = n_blocks;
    p->block_size = block_size;
    for (int i = 0; i < n_blocks; i++) {
        p->blocks[i].data = (char *)p->backing + (size_t)i * block_size;
        p->blocks[i].size = block_size;
        p->blocks[i].in_use = false;
    }
    return p;
}

void bp_pool_destroy(bp_pool_t *p) {
    if (!p)
        return;
    free(p->backing);
    free(p);
}

bp_block_t *bp_pool_acquire(bp_pool_t *p) {
    if (!p)
        return NULL;
    for (int i = 0; i < p->n_blocks; i++) {
        if (!p->blocks[i].in_use) {
            p->blocks[i].in_use = true;
            p->in_use++;
            if (p->in_use > p->peak)
                p->peak = p->in_use;
            return &p->blocks[i];
        }
    }
    return NULL; /* pool exhausted */
}

int bp_pool_release(bp_pool_t *p, bp_block_t *b) {
    if (!p || !b)
        return -1;
    /* Verify block belongs to this pool */
    if (b < p->blocks || b >= p->blocks + p->n_blocks)
        return -1;
    if (!b->in_use)
        return -1;
    b->in_use = false;
    p->in_use--;
    return 0;
}

int bp_pool_in_use(const bp_pool_t *p) {
    return p ? p->in_use : 0;
}
int bp_pool_peak(const bp_pool_t *p) {
    return p ? p->peak : 0;
}
int bp_pool_capacity(const bp_pool_t *p) {
    return p ? p->n_blocks : 0;
}
