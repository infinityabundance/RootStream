/*
 * kfr_message.h — Keyframe request message wire format
 *
 * Supports two RTCP-derived request types:
 *   PLI  (Picture Loss Indication, RFC 4585 §6.3.1)
 *   FIR  (Full Intra Request, RFC 5104 §4.3.1)
 *
 * Wire layout (little-endian)
 * ───────────────────────────
 *  Offset  Size  Field
 *   0      4     Magic        0x4B465251 ('KFRQ')
 *   4      1     Type         KFR_TYPE_PLI or KFR_TYPE_FIR
 *   5      1     Priority     0 = normal, 1 = urgent (e.g. heavy loss)
 *   6      2     Seq          monotonic request sequence number
 *   8      4     SSRC         stream SSRC targeted by this request
 *  12      8     Timestamp_us request timestamp in µs
 *  20      4     Reserved (0)
 *  24            End (24 bytes total)
 *
 * Thread-safety: stateless encode/decode — thread-safe.
 */

#ifndef ROOTSTREAM_KFR_MESSAGE_H
#define ROOTSTREAM_KFR_MESSAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KFR_MSG_MAGIC 0x4B465251UL /* 'KFRQ' */
#define KFR_MSG_SIZE 24

/** Keyframe request type */
typedef enum {
    KFR_TYPE_PLI = 1, /**< Picture Loss Indication */
    KFR_TYPE_FIR = 2, /**< Full Intra Request */
} kfr_type_t;

/** Keyframe request message */
typedef struct {
    kfr_type_t type;
    uint8_t priority;
    uint16_t seq;
    uint32_t ssrc;
    uint64_t timestamp_us;
} kfr_message_t;

/**
 * kfr_message_encode — serialise @msg into @buf
 *
 * @param msg     Message to encode
 * @param buf     Output buffer (>= KFR_MSG_SIZE)
 * @param buf_sz  Buffer size
 * @return        KFR_MSG_SIZE on success, -1 on error
 */
int kfr_message_encode(const kfr_message_t *msg, uint8_t *buf, size_t buf_sz);

/**
 * kfr_message_decode — parse @msg from @buf
 *
 * @param buf     Input buffer
 * @param buf_sz  Valid bytes
 * @param msg     Output message
 * @return        0 on success, -1 on error
 */
int kfr_message_decode(const uint8_t *buf, size_t buf_sz, kfr_message_t *msg);

/**
 * kfr_type_name — human-readable type name
 *
 * @param t  Request type
 * @return   Static string
 */
const char *kfr_type_name(kfr_type_t t);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_KFR_MESSAGE_H */
