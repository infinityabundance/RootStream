/*
 * probe_packet.h — Bandwidth probe packet wire format
 *
 * Each probe PDU is a fixed 32-byte record sent by the prober and
 * received (optionally echoed) by the far end.  One-way delay and
 * inter-arrival gap measurements are derived from the timestamps.
 *
 * Wire layout (little-endian)
 * ───────────────────────────
 *  Offset  Size  Field
 *   0      4     Magic      0x50524F42 ('PROB')
 *   4      2     Seq        monotonic sequence number
 *   6      2     Size hint  nominal packet size in bytes (for cross-traffic sim)
 *   8      8     Send ts    sender timestamp in µs
 *  16      4     Burst ID   probe burst this packet belongs to
 *  20      4     Burst seq  position within burst (0-based)
 *  24      8     Reserved (0)
 *
 * Thread-safety: stateless encode/decode — thread-safe.
 */

#ifndef ROOTSTREAM_PROBE_PACKET_H
#define ROOTSTREAM_PROBE_PACKET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROBE_PKT_MAGIC 0x50524F42UL /* 'PROB' */
#define PROBE_PKT_SIZE 32

/** Probe packet */
typedef struct {
    uint16_t seq;
    uint16_t size_hint;
    uint64_t send_ts_us;
    uint32_t burst_id;
    uint32_t burst_seq;
} probe_packet_t;

/**
 * probe_packet_encode — serialise @pkt into @buf
 *
 * @param pkt     Packet to encode
 * @param buf     Output buffer (>= PROBE_PKT_SIZE)
 * @param buf_sz  Buffer size
 * @return        PROBE_PKT_SIZE on success, -1 on error
 */
int probe_packet_encode(const probe_packet_t *pkt, uint8_t *buf, size_t buf_sz);

/**
 * probe_packet_decode — parse @pkt from @buf
 *
 * @param buf     Input buffer
 * @param buf_sz  Valid bytes
 * @param pkt     Output packet
 * @return        0 on success, -1 on error
 */
int probe_packet_decode(const uint8_t *buf, size_t buf_sz, probe_packet_t *pkt);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PROBE_PACKET_H */
