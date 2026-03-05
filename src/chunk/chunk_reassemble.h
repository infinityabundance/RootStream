/*
 * chunk_reassemble.h — Chunk Splitter: frame reassembly
 *
 * Maintains up to REASSEMBLE_SLOTS independent reassembly slots, each
 * tracking one in-progress frame identified by (stream_id, frame_seq).
 * As chunks arrive the received bitmask is updated; when all
 * chunk_count chunks have been received the slot is marked complete.
 *
 * The caller is responsible for allocating and freeing payload storage
 * — the reassembler only tracks arrival state.
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_CHUNK_REASSEMBLE_H
#define ROOTSTREAM_CHUNK_REASSEMBLE_H

#include "chunk_hdr.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REASSEMBLE_SLOTS      8    /**< Concurrent reassembly slots */
#define REASSEMBLE_MAX_CHUNKS 32   /**< Max chunks per frame (≤ 32 for bitmask) */

/** One reassembly slot */
typedef struct {
    uint32_t stream_id;
    uint32_t frame_seq;
    uint16_t chunk_count;           /**< Expected total chunks */
    uint32_t received_mask;         /**< Bit i set when chunk i arrived */
    bool     complete;
    bool     in_use;
} reassemble_slot_t;

/** Opaque reassembly context */
typedef struct reassemble_ctx_s reassemble_ctx_t;

/**
 * reassemble_ctx_create — allocate context
 *
 * @return Non-NULL handle, or NULL on OOM
 */
reassemble_ctx_t *reassemble_ctx_create(void);

/**
 * reassemble_ctx_destroy — free context
 */
void reassemble_ctx_destroy(reassemble_ctx_t *c);

/**
 * reassemble_receive — record arrival of one chunk
 *
 * Opens a new slot for unseen (stream_id, frame_seq) pairs.
 * Returns NULL if chunk_count > REASSEMBLE_MAX_CHUNKS or no free slot.
 *
 * @param c    Context
 * @param h    Chunk header (chunk_idx, chunk_count, stream_id, frame_seq)
 * @return     Pointer to the slot (owned by context); complete flag set
 *             when all chunks received
 */
reassemble_slot_t *reassemble_receive(reassemble_ctx_t *c,
                                       const chunk_hdr_t *h);

/**
 * reassemble_release — free a slot after it has been fully processed
 *
 * @param c  Context
 * @param s  Slot returned by reassemble_receive()
 * @return   0 on success, -1 if not found
 */
int reassemble_release(reassemble_ctx_t *c, reassemble_slot_t *s);

/**
 * reassemble_count — number of active (in-use) slots
 */
int reassemble_count(const reassemble_ctx_t *c);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CHUNK_REASSEMBLE_H */
