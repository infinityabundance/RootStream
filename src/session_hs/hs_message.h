/*
 * hs_message.h — Handshake message wire format
 *
 * Every handshake PDU shares a fixed 16-byte header followed by a
 * variable-length payload (up to HS_MAX_PAYLOAD bytes).
 *
 * Wire layout (little-endian)
 * ───────────────────────────
 *  Offset  Size  Field
 *   0      4     Magic        0x48534D47 ('HSMG')
 *   4      2     Type         hs_msg_type_t
 *   6      2     Seq          monotonic PDU sequence number
 *   8      2     Payload len  bytes following header
 *  10      2     Reserved (0)
 *  12      4     CRC32        CRC of bytes [0..11] + payload
 *  16      N     Payload      N = payload_len
 *
 * Thread-safety: stateless encode/decode — thread-safe.
 */

#ifndef ROOTSTREAM_HS_MESSAGE_H
#define ROOTSTREAM_HS_MESSAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HS_MSG_MAGIC 0x48534D47UL /* 'HSMG' */
#define HS_MSG_HDR_SIZE 16
#define HS_MAX_PAYLOAD 256

/** Handshake PDU types */
typedef enum {
    HS_MSG_HELLO = 1,     /**< Client → Server: initiate session */
    HS_MSG_HELLO_ACK = 2, /**< Server → Client: send session token */
    HS_MSG_AUTH = 3,      /**< Client → Server: authenticate */
    HS_MSG_AUTH_ACK = 4,  /**< Server → Client: auth result */
    HS_MSG_CONFIG = 5,    /**< Server → Client: stream config */
    HS_MSG_READY = 6,     /**< Bidirectional: stream ready */
    HS_MSG_ERROR = 7,     /**< Any direction: error and reason */
    HS_MSG_BYE = 8,       /**< Bidirectional: graceful disconnect */
} hs_msg_type_t;

/** In-memory handshake message */
typedef struct {
    hs_msg_type_t type;
    uint16_t seq;
    uint16_t payload_len;
    uint8_t payload[HS_MAX_PAYLOAD];
} hs_message_t;

/**
 * hs_message_encode — serialise @msg into @buf
 *
 * Computes and embeds the CRC32 of header + payload.
 *
 * @param msg     Message to encode
 * @param buf     Output buffer (>= HS_MSG_HDR_SIZE + msg->payload_len)
 * @param buf_sz  Buffer size
 * @return        Bytes written, or -1 on error
 */
int hs_message_encode(const hs_message_t *msg, uint8_t *buf, size_t buf_sz);

/**
 * hs_message_decode — parse @msg from @buf
 *
 * Validates magic and CRC32.
 *
 * @param buf     Input buffer
 * @param buf_sz  Valid bytes in @buf
 * @param msg     Output message
 * @return        0 on success, -1 on error
 */
int hs_message_decode(const uint8_t *buf, size_t buf_sz, hs_message_t *msg);

/**
 * hs_msg_type_name — human-readable type name
 *
 * @param t  Message type
 * @return   Static string
 */
const char *hs_msg_type_name(hs_msg_type_t t);

/**
 * hs_message_total_size — encoded size for a given payload length
 *
 * @param payload_len  Payload bytes
 * @return             HS_MSG_HDR_SIZE + payload_len
 */
static inline int hs_message_total_size(uint16_t payload_len) {
    return (int)(HS_MSG_HDR_SIZE + (int)payload_len);
}

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_HS_MESSAGE_H */
