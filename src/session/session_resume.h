/*
 * session_resume.h — Session resume-protocol negotiation
 *
 * Defines the handshake used by a reconnecting client to resume an
 * interrupted streaming session without an IDR round-trip penalty.
 *
 * Resume handshake (both client and server must agree):
 *   1. Client sends RESUME_REQUEST with session_id + last_frame
 *   2. Server validates: session exists + last_frame <= server last_keyframe
 *   3. Server replies RESUME_ACCEPTED (stream continues from keyframe)
 *      or RESUME_REJECTED (full reconnect required)
 *
 * Wire format — all integers little-endian, messages prefixed with a
 * 4-byte tag and 4-byte length.
 */

#ifndef ROOTSTREAM_SESSION_RESUME_H
#define ROOTSTREAM_SESSION_RESUME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "session_state.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RESUME_TAG_REQUEST 0x52455351UL  /* 'RESQ' */
#define RESUME_TAG_ACCEPTED 0x52455341UL /* 'RESA' */
#define RESUME_TAG_REJECTED 0x52455352UL /* 'RESR' */
#define RESUME_MSG_HDR_SIZE 8            /* tag(4) + length(4) */

/** Reasons a resume might be rejected */
typedef enum {
    RESUME_REJECT_UNKNOWN_SESSION = 1,
    RESUME_REJECT_FRAME_GAP_TOO_LARGE = 2,
    RESUME_REJECT_STATE_MISMATCH = 3,
} resume_reject_reason_t;

/** RESUME_REQUEST payload (client → server) */
typedef struct {
    uint64_t session_id;
    uint64_t last_frame_received; /**< Highest frame the client decoded */
    uint8_t stream_key[SESSION_STREAM_KEY_LEN];
} resume_request_t;

/** RESUME_ACCEPTED payload (server → client) */
typedef struct {
    uint64_t session_id;
    uint32_t resume_from_frame; /**< Server will re-send from this frame */
    uint32_t bitrate_kbps;      /**< Suggested starting bitrate */
} resume_accepted_t;

/** RESUME_REJECTED payload (server → client) */
typedef struct {
    uint64_t session_id;
    resume_reject_reason_t reason;
} resume_rejected_t;

/**
 * resume_encode_request — serialise a RESUME_REQUEST into @buf
 *
 * @param req     Request to encode
 * @param buf     Output buffer (must be >= RESUME_MSG_HDR_SIZE + 72)
 * @return        Bytes written, or -1 on error
 */
int resume_encode_request(const resume_request_t *req, uint8_t *buf, size_t buf_sz);

/**
 * resume_decode_request — parse a RESUME_REQUEST from @buf
 *
 * @param buf     Input buffer
 * @param buf_sz  Length of @buf
 * @param req     Output request
 * @return        0 on success, -1 on parse error
 */
int resume_decode_request(const uint8_t *buf, size_t buf_sz, resume_request_t *req);

/**
 * resume_encode_accepted — serialise a RESUME_ACCEPTED into @buf
 *
 * @return  Bytes written, or -1 on error
 */
int resume_encode_accepted(const resume_accepted_t *acc, uint8_t *buf, size_t buf_sz);

/**
 * resume_decode_accepted — parse a RESUME_ACCEPTED from @buf
 *
 * @return  0 on success, -1 on parse error
 */
int resume_decode_accepted(const uint8_t *buf, size_t buf_sz, resume_accepted_t *acc);

/**
 * resume_encode_rejected — serialise a RESUME_REJECTED into @buf
 *
 * @return  Bytes written, or -1 on error
 */
int resume_encode_rejected(const resume_rejected_t *rej, uint8_t *buf, size_t buf_sz);

/**
 * resume_decode_rejected — parse a RESUME_REJECTED from @buf
 *
 * @return  0 on success, -1 on parse error
 */
int resume_decode_rejected(const uint8_t *buf, size_t buf_sz, resume_rejected_t *rej);

/**
 * resume_server_evaluate — decide whether to accept a resume request
 *
 * @param req            Client's resume request
 * @param server_state   Server's stored session state (from checkpoint)
 * @param max_frame_gap  Maximum allowed gap between client and server frames
 * @param out_acc        Populated on ACCEPT decision (may be NULL)
 * @param out_rej        Populated on REJECT decision (may be NULL)
 * @return               true if accepted, false if rejected
 */
bool resume_server_evaluate(const resume_request_t *req, const session_state_t *server_state,
                            uint32_t max_frame_gap, resume_accepted_t *out_acc,
                            resume_rejected_t *out_rej);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SESSION_RESUME_H */
