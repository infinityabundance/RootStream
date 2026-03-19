/*
 * pointer_sync.h — Remote cursor / pointer synchronisation
 *
 * Tracks the normalised pointer position of each remote peer and
 * provides a snapshot suitable for overlay rendering.  Pointers
 * time out after a configurable idle period.
 */

#ifndef ROOTSTREAM_POINTER_SYNC_H
#define ROOTSTREAM_POINTER_SYNC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "annotation_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of simultaneously tracked remote pointers */
#define POINTER_SYNC_MAX_PEERS 16

/** Default idle timeout before pointer is considered hidden (µs) */
#define POINTER_SYNC_TIMEOUT_US (3000000ULL) /* 3 seconds */

/** Snapshot of one remote pointer */
typedef struct {
    uint32_t peer_id;
    annot_point_t pos;
    uint64_t last_updated_us; /**< Monotonic timestamp */
    bool visible;
} remote_pointer_t;

/** Opaque pointer sync state */
typedef struct pointer_sync_s pointer_sync_t;

/**
 * pointer_sync_create — allocate pointer sync state
 *
 * @param timeout_us  Idle timeout in µs (0 = use default)
 * @return            Non-NULL handle, or NULL on OOM
 */
pointer_sync_t *pointer_sync_create(uint64_t timeout_us);

/**
 * pointer_sync_destroy — free pointer sync state
 *
 * @param ps  State to destroy
 */
void pointer_sync_destroy(pointer_sync_t *ps);

/**
 * pointer_sync_update — process an annotation event for pointer state
 *
 * Only ANNOT_POINTER_MOVE and ANNOT_POINTER_HIDE are handled; others
 * are silently ignored.
 *
 * @param ps     Pointer sync state
 * @param event  Annotation event
 */
void pointer_sync_update(pointer_sync_t *ps, const annotation_event_t *event);

/**
 * pointer_sync_get — retrieve the current state of a peer's pointer
 *
 * @param ps      Pointer sync state
 * @param peer_id Remote peer identifier
 * @param out     Receives the pointer snapshot
 * @return        0 if found, -1 if not tracked
 */
int pointer_sync_get(const pointer_sync_t *ps, uint32_t peer_id, remote_pointer_t *out);

/**
 * pointer_sync_get_all — fill @out with all currently visible pointers
 *
 * Pointers that have timed out are excluded.
 *
 * @param ps         Pointer sync state
 * @param out        Array to fill
 * @param max_count  Capacity of @out
 * @return           Number of visible pointers written
 */
int pointer_sync_get_all(const pointer_sync_t *ps, remote_pointer_t *out, int max_count);

/**
 * pointer_sync_expire — remove pointers that have exceeded the timeout
 *
 * Call periodically (e.g. once per rendered frame).
 *
 * @param ps          Pointer sync state
 * @param now_us      Current monotonic timestamp in µs
 */
void pointer_sync_expire(pointer_sync_t *ps, uint64_t now_us);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_POINTER_SYNC_H */
