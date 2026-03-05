/*
 * session_state.h — Session state serialisation for persistence
 *
 * Captures a snapshot of runtime session parameters that are needed
 * to resume streaming after a reconnect or process restart.  The
 * snapshot is serialised to a compact binary format suitable for
 * writing to a checkpoint file or sending over the network.
 *
 * Binary layout (little-endian throughout):
 * ──────────────────────────────────────────
 *  Offset  Size  Field
 *   0      4     Magic  0x52535353 ('RSSS')
 *   4      2     Format version (1)
 *   6      2     Flags
 *   8      8     Session ID (uint64)
 *  16      8     Created timestamp (µs monotonic)
 *  24      4     Width  (pixels)
 *  28      4     Height (pixels)
 *  32      4     Framerate numerator
 *  36      4     Framerate denominator
 *  40      4     Bitrate (kbps)
 *  44      4     Audio sample rate
 *  48      4     Audio channels
 *  52      4     Last keyframe number
 *  56      8     Total frames sent
 *  64     32     Stream key (opaque, e.g. BLAKE2 derivation)
 *  96      2     Peer address length
 *  98      N     Peer address (UTF-8, up to SESSION_MAX_PEER_ADDR bytes)
 */

#ifndef ROOTSTREAM_SESSION_STATE_H
#define ROOTSTREAM_SESSION_STATE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SESSION_STATE_MAGIC       0x52535353UL  /* 'RSSS' */
#define SESSION_STATE_VERSION     1
#define SESSION_MAX_PEER_ADDR     64
#define SESSION_STREAM_KEY_LEN    32
#define SESSION_STATE_MIN_SIZE    100  /* header without peer addr */
#define SESSION_STATE_MAX_SIZE    (SESSION_STATE_MIN_SIZE + SESSION_MAX_PEER_ADDR)

/** Complete session snapshot */
typedef struct {
    uint64_t session_id;
    uint64_t created_us;          /**< Monotonic creation timestamp */
    uint32_t width;
    uint32_t height;
    uint32_t fps_num;             /**< Framerate numerator */
    uint32_t fps_den;             /**< Framerate denominator */
    uint32_t bitrate_kbps;
    uint32_t audio_sample_rate;
    uint32_t audio_channels;
    uint32_t last_keyframe;       /**< Frame number of last IDR */
    uint64_t frames_sent;
    uint8_t  stream_key[SESSION_STREAM_KEY_LEN];
    char     peer_addr[SESSION_MAX_PEER_ADDR + 1]; /**< NUL-terminated */
    uint16_t flags;               /**< Reserved for future use */
} session_state_t;

/**
 * session_state_serialise — encode @state into @buf
 *
 * @param state   Session state to serialise
 * @param buf     Output buffer
 * @param buf_sz  Size of @buf (must be >= SESSION_STATE_MAX_SIZE)
 * @return        Number of bytes written, or -1 on error
 */
int session_state_serialise(const session_state_t *state,
                             uint8_t               *buf,
                             size_t                 buf_sz);

/**
 * session_state_deserialise — decode @state from @buf
 *
 * @param buf      Input buffer
 * @param buf_sz   Number of valid bytes in @buf
 * @param state    Output session state
 * @return         0 on success, -1 on bad magic/version/overflow
 */
int session_state_deserialise(const uint8_t   *buf,
                               size_t           buf_sz,
                               session_state_t *state);

/**
 * session_state_serialised_size — return exact size for @state
 *
 * @param state  Session state
 * @return       Byte count that session_state_serialise will write
 */
size_t session_state_serialised_size(const session_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SESSION_STATE_H */
