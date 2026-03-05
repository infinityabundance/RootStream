/*
 * bp_block.h — Fixed-size buffer pool block
 *
 * A block owns a fixed-length byte array allocated at pool creation
 * time.  The `in_use` flag tracks whether the block is currently held
 * by a caller.  The pool manages the full array; individual blocks are
 * handed to callers via acquire/release.
 *
 * Thread-safety: value type — no shared state; the pool is not
 * thread-safe.
 */

#ifndef ROOTSTREAM_BP_BLOCK_H
#define ROOTSTREAM_BP_BLOCK_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Single buffer pool block */
typedef struct {
    void   *data;     /**< Pointer into pool's backing store */
    size_t  size;     /**< Block size in bytes */
    bool    in_use;
} bp_block_t;

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_BP_BLOCK_H */
