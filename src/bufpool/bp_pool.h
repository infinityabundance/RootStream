/*
 * bp_pool.h — Fixed-size buffer pool
 *
 * Allocates N blocks of `block_size` bytes in a single contiguous
 * backing store at creation time.  `bp_pool_acquire()` returns an
 * unused block (O(N) scan); `bp_pool_release()` marks it free.
 * The pool tracks the high-water mark (peak_in_use).
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_BP_POOL_H
#define ROOTSTREAM_BP_POOL_H

#include <stddef.h>

#include "bp_block.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BP_MAX_BLOCKS 64 /**< Maximum blocks per pool */

/** Opaque buffer pool */
typedef struct bp_pool_s bp_pool_t;

/**
 * bp_pool_create — allocate pool
 *
 * @param n_blocks    Number of blocks (1..BP_MAX_BLOCKS)
 * @param block_size  Size of each block in bytes (> 0)
 * @return            Non-NULL handle, or NULL on OOM/invalid
 */
bp_pool_t *bp_pool_create(int n_blocks, size_t block_size);

/**
 * bp_pool_destroy — free pool and all backing memory
 */
void bp_pool_destroy(bp_pool_t *p);

/**
 * bp_pool_acquire — get a free block
 *
 * @param p  Pool
 * @return   Pointer to free bp_block_t (data field ready to use),
 *           or NULL if all blocks are in use
 */
bp_block_t *bp_pool_acquire(bp_pool_t *p);

/**
 * bp_pool_release — return a block to the pool
 *
 * @param p  Pool
 * @param b  Block previously returned by bp_pool_acquire()
 * @return   0 on success, -1 on NULL or block not from this pool
 */
int bp_pool_release(bp_pool_t *p, bp_block_t *b);

/**
 * bp_pool_in_use — number of currently acquired blocks
 */
int bp_pool_in_use(const bp_pool_t *p);

/**
 * bp_pool_peak — high-water mark of simultaneous in-use blocks
 */
int bp_pool_peak(const bp_pool_t *p);

/**
 * bp_pool_capacity — total number of blocks
 */
int bp_pool_capacity(const bp_pool_t *p);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_BP_POOL_H */
