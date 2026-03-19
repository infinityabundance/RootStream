/*
 * reorder_slot.h — Reorder buffer slot
 *
 * Each slot holds a reference to one packet identified by its 16-bit
 * sequence number plus the arrival timestamp used for timeout flushing.
 *
 * The payload is opaque (caller-managed byte buffer).
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_REORDER_SLOT_H
#define ROOTSTREAM_REORDER_SLOT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REORDER_SLOT_MAX_PAYLOAD 2048 /**< Maximum payload bytes per slot */

/** Reorder slot */
typedef struct {
    uint16_t seq;        /**< RTP-style sequence number */
    uint64_t arrival_us; /**< Arrival timestamp in µs */
    uint8_t payload[REORDER_SLOT_MAX_PAYLOAD];
    uint16_t payload_len; /**< Valid payload bytes */
    bool occupied;        /**< Slot contains a packet */
} reorder_slot_t;

/**
 * reorder_slot_fill — populate slot from raw data
 *
 * @param slot        Slot to fill
 * @param seq         Sequence number
 * @param arrival_us  Arrival timestamp in µs
 * @param payload     Packet payload (copied)
 * @param payload_len Payload length (must be <= REORDER_SLOT_MAX_PAYLOAD)
 * @return            0 on success, -1 on error
 */
int reorder_slot_fill(reorder_slot_t *slot, uint16_t seq, uint64_t arrival_us,
                      const uint8_t *payload, uint16_t payload_len);

/**
 * reorder_slot_clear — reset slot to empty
 *
 * @param slot  Slot to clear
 */
void reorder_slot_clear(reorder_slot_t *slot);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_REORDER_SLOT_H */
