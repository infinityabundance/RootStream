/*
 * fanout_manager.h — Multi-client fanout streaming manager
 *
 * The fanout manager accepts a single encoded video/audio frame and
 * delivers it to all registered sessions.  It sits between the encoder
 * output and the transport layer.
 *
 * For each session the manager:
 *   1. Checks per-client ABR headroom (drop frame if client is congested)
 *   2. Sends the frame on the session's socket (or queues it)
 *   3. Updates per-client statistics
 *
 * Thread-safety: fanout_manager_deliver() is safe to call from any single
 * producer thread.  session management functions (add/remove) lock
 * internally and may be called from any thread.
 */

#ifndef ROOTSTREAM_FANOUT_MANAGER_H
#define ROOTSTREAM_FANOUT_MANAGER_H

#include "session_table.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Frame type tags */
typedef enum {
    FANOUT_FRAME_VIDEO_KEY   = 0,  /**< IDR / keyframe */
    FANOUT_FRAME_VIDEO_DELTA = 1,  /**< P- or B-frame */
    FANOUT_FRAME_AUDIO       = 2,
    FANOUT_FRAME_DATA        = 3,  /**< Control / metadata */
} fanout_frame_type_t;

/** Delivery statistics snapshot */
typedef struct {
    uint64_t frames_in;        /**< Total frames submitted */
    uint64_t frames_delivered; /**< Total frames sent to ≥1 client */
    uint64_t frames_dropped;   /**< Total frames dropped (congestion) */
    size_t   active_sessions;  /**< Current active session count */
} fanout_stats_t;

/** Opaque fanout manager handle */
typedef struct fanout_manager_s fanout_manager_t;

/**
 * fanout_manager_create — allocate fanout manager
 *
 * @param table  Session table to use for session lookup (not owned)
 * @return       Non-NULL handle, or NULL on OOM / bad args
 */
fanout_manager_t *fanout_manager_create(session_table_t *table);

/**
 * fanout_manager_destroy — free fanout manager resources
 *
 * @param mgr  Manager to destroy
 */
void fanout_manager_destroy(fanout_manager_t *mgr);

/**
 * fanout_manager_deliver — fan @frame_data out to all active sessions
 *
 * Keyframes are always delivered.  Delta frames are dropped for a
 * session only when the client's estimated bandwidth cannot sustain
 * the current frame rate at its negotiated bitrate.
 *
 * @param mgr         Fanout manager
 * @param frame_data  Encoded frame payload
 * @param frame_size  Payload size in bytes
 * @param type        Frame type (key/delta/audio/data)
 * @return            Number of sessions the frame was delivered to
 */
int fanout_manager_deliver(fanout_manager_t   *mgr,
                           const uint8_t      *frame_data,
                           size_t              frame_size,
                           fanout_frame_type_t type);

/**
 * fanout_manager_get_stats — retrieve delivery statistics
 *
 * @param mgr   Fanout manager
 * @param stats Output statistics snapshot
 */
void fanout_manager_get_stats(const fanout_manager_t *mgr,
                               fanout_stats_t         *stats);

/**
 * fanout_manager_reset_stats — zero all delivery counters
 *
 * @param mgr  Fanout manager
 */
void fanout_manager_reset_stats(fanout_manager_t *mgr);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_FANOUT_MANAGER_H */
