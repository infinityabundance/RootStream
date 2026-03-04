/*
 * session_resume.c — Session resume protocol implementation
 */

#include "session_resume.h"

#include <string.h>

/* ── Little-endian write/read helpers ────────────────────────────── */

static void w32le(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
static void w64le(uint8_t *p, uint64_t v) {
    for (int i=0;i<8;i++) p[i]=(uint8_t)(v>>(i*8));
}
static uint32_t r32le(const uint8_t *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)
          |((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint64_t r64le(const uint8_t *p) {
    uint64_t v=0;
    for(int i=0;i<8;i++) v|=((uint64_t)p[i]<<(i*8));
    return v;
}

/* ── RESUME_REQUEST ──────────────────────────────────────────────── */

#define REQUEST_PAYLOAD_SZ  (8 + 8 + SESSION_STREAM_KEY_LEN)   /* 48 */
#define ACCEPTED_PAYLOAD_SZ (8 + 4 + 4)                        /* 16 */
#define REJECTED_PAYLOAD_SZ (8 + 4)                            /* 12 */

int resume_encode_request(const resume_request_t *req,
                           uint8_t                *buf,
                           size_t                  buf_sz) {
    if (!req || !buf) return -1;
    size_t total = RESUME_MSG_HDR_SIZE + REQUEST_PAYLOAD_SZ;
    if (buf_sz < total) return -1;

    w32le(buf, (uint32_t)RESUME_TAG_REQUEST);
    w32le(buf + 4, REQUEST_PAYLOAD_SZ);
    w64le(buf + 8,  req->session_id);
    w64le(buf + 16, req->last_frame_received);
    memcpy(buf + 24, req->stream_key, SESSION_STREAM_KEY_LEN);
    return (int)total;
}

int resume_decode_request(const uint8_t    *buf,
                           size_t            buf_sz,
                           resume_request_t *req) {
    if (!buf || !req || buf_sz < RESUME_MSG_HDR_SIZE + REQUEST_PAYLOAD_SZ)
        return -1;
    if (r32le(buf) != (uint32_t)RESUME_TAG_REQUEST) return -1;

    req->session_id          = r64le(buf + 8);
    req->last_frame_received = r64le(buf + 16);
    memcpy(req->stream_key, buf + 24, SESSION_STREAM_KEY_LEN);
    return 0;
}

/* ── RESUME_ACCEPTED ─────────────────────────────────────────────── */

int resume_encode_accepted(const resume_accepted_t *acc,
                            uint8_t                 *buf,
                            size_t                   buf_sz) {
    if (!acc || !buf) return -1;
    size_t total = RESUME_MSG_HDR_SIZE + ACCEPTED_PAYLOAD_SZ;
    if (buf_sz < total) return -1;

    w32le(buf, (uint32_t)RESUME_TAG_ACCEPTED);
    w32le(buf + 4, ACCEPTED_PAYLOAD_SZ);
    w64le(buf + 8,  acc->session_id);
    w32le(buf + 16, acc->resume_from_frame);
    w32le(buf + 20, acc->bitrate_kbps);
    return (int)total;
}

int resume_decode_accepted(const uint8_t     *buf,
                            size_t             buf_sz,
                            resume_accepted_t *acc) {
    if (!buf || !acc || buf_sz < RESUME_MSG_HDR_SIZE + ACCEPTED_PAYLOAD_SZ)
        return -1;
    if (r32le(buf) != (uint32_t)RESUME_TAG_ACCEPTED) return -1;

    acc->session_id       = r64le(buf + 8);
    acc->resume_from_frame = r32le(buf + 16);
    acc->bitrate_kbps     = r32le(buf + 20);
    return 0;
}

/* ── RESUME_REJECTED ─────────────────────────────────────────────── */

int resume_encode_rejected(const resume_rejected_t *rej,
                            uint8_t                 *buf,
                            size_t                   buf_sz) {
    if (!rej || !buf) return -1;
    size_t total = RESUME_MSG_HDR_SIZE + REJECTED_PAYLOAD_SZ;
    if (buf_sz < total) return -1;

    w32le(buf, (uint32_t)RESUME_TAG_REJECTED);
    w32le(buf + 4, REJECTED_PAYLOAD_SZ);
    w64le(buf + 8,  rej->session_id);
    w32le(buf + 16, (uint32_t)rej->reason);
    return (int)total;
}

int resume_decode_rejected(const uint8_t     *buf,
                            size_t             buf_sz,
                            resume_rejected_t *rej) {
    if (!buf || !rej || buf_sz < RESUME_MSG_HDR_SIZE + REJECTED_PAYLOAD_SZ)
        return -1;
    if (r32le(buf) != (uint32_t)RESUME_TAG_REJECTED) return -1;

    rej->session_id = r64le(buf + 8);
    rej->reason     = (resume_reject_reason_t)r32le(buf + 16);
    return 0;
}

/* ── Server evaluation ───────────────────────────────────────────── */

bool resume_server_evaluate(const resume_request_t  *req,
                             const session_state_t   *server_state,
                             uint32_t                 max_frame_gap,
                             resume_accepted_t       *out_acc,
                             resume_rejected_t       *out_rej) {
    if (!req || !server_state) {
        if (out_rej) {
            out_rej->session_id = req ? req->session_id : 0;
            out_rej->reason     = RESUME_REJECT_UNKNOWN_SESSION;
        }
        return false;
    }

    /* Session ID must match */
    if (req->session_id != server_state->session_id) {
        if (out_rej) {
            out_rej->session_id = req->session_id;
            out_rej->reason     = RESUME_REJECT_UNKNOWN_SESSION;
        }
        return false;
    }

    /* Stream key must match */
    if (memcmp(req->stream_key, server_state->stream_key,
               SESSION_STREAM_KEY_LEN) != 0) {
        if (out_rej) {
            out_rej->session_id = req->session_id;
            out_rej->reason     = RESUME_REJECT_STATE_MISMATCH;
        }
        return false;
    }

    /* Frame gap check */
    uint64_t gap = 0;
    if (server_state->frames_sent > req->last_frame_received) {
        gap = server_state->frames_sent - req->last_frame_received;
    }
    if (gap > max_frame_gap) {
        if (out_rej) {
            out_rej->session_id = req->session_id;
            out_rej->reason     = RESUME_REJECT_FRAME_GAP_TOO_LARGE;
        }
        return false;
    }

    if (out_acc) {
        out_acc->session_id        = req->session_id;
        out_acc->resume_from_frame = server_state->last_keyframe;
        out_acc->bitrate_kbps      = server_state->bitrate_kbps;
    }
    return true;
}
