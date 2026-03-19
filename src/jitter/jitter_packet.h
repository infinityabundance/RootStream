/*
 * jitter_packet.h — Jitter buffer packet format
 *
 * A jitter_packet_t wraps a payload with sequence number, RTP-style
 * timestamp, and capture time so the jitter buffer can reorder and
 * hold packets for the correct playout delay.
 *
 * Wire encoding (little-endian)
 * ──────────────────────────────
 *  Offset  Size  Field
 *   0      4     Magic    0x4A504B54 ('JPKT')
 *   4      4     seq_num  — 32-bit sequence number
 *   8      4     rtp_ts   — 32-bit RTP timestamp (90 kHz clock)
 *  12      8     capture_us — capture timestamp (µs epoch)
 *  20      2     payload_len
 *  22      1     payload_type
 *  23      1     flags
 *  24      N     payload (up to JITTER_MAX_PAYLOAD bytes)
 */

#ifndef ROOTSTREAM_JITTER_PACKET_H
#define ROOTSTREAM_JITTER_PACKET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JITTER_MAGIC 0x4A504B54UL /* 'JPKT' */
#define JITTER_PKT_HDR_SIZE 24
#define JITTER_MAX_PAYLOAD 1400 /* MTU-safe payload bytes */

/** Jitter buffer packet */
typedef struct {
    uint32_t seq_num;
    uint32_t rtp_ts;
    uint64_t capture_us;
    uint16_t payload_len;
    uint8_t payload_type;
    uint8_t flags;
    uint8_t payload[JITTER_MAX_PAYLOAD];
} jitter_packet_t;

/**
 * jitter_packet_encode — serialise @pkt into @buf
 *
 * @param pkt     Packet to encode
 * @param buf     Output buffer (>= JITTER_PKT_HDR_SIZE + payload_len)
 * @param buf_sz  Buffer size
 * @return        Bytes written, or -1 on error
 */
int jitter_packet_encode(const jitter_packet_t *pkt, uint8_t *buf, size_t buf_sz);

/**
 * jitter_packet_decode — parse @pkt from @buf
 *
 * @param buf     Input buffer
 * @param buf_sz  Valid bytes in @buf
 * @param pkt     Output packet
 * @return        0 on success, -1 on error
 */
int jitter_packet_decode(const uint8_t *buf, size_t buf_sz, jitter_packet_t *pkt);

/**
 * jitter_packet_before — return true if @a arrives before @b (seq-num order)
 *
 * Uses a 32-bit sequence number with half-window wrap handling so that
 * seq 0 is "before" seq UINT32_MAX/2.
 *
 * @param a  Sequence number a
 * @param b  Sequence number b
 * @return   true if a should playout before b
 */
bool jitter_packet_before(uint32_t a, uint32_t b);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_JITTER_PACKET_H */
