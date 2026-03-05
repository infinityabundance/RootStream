/*
 * reorder_slot.c — Reorder buffer slot implementation
 */

#include "reorder_slot.h"

#include <string.h>

int reorder_slot_fill(reorder_slot_t *slot,
                       uint16_t        seq,
                       uint64_t        arrival_us,
                       const uint8_t  *payload,
                       uint16_t        payload_len) {
    if (!slot) return -1;
    if (payload_len > REORDER_SLOT_MAX_PAYLOAD) return -1;
    if (payload_len > 0 && !payload) return -1;

    slot->seq         = seq;
    slot->arrival_us  = arrival_us;
    slot->payload_len = payload_len;
    slot->occupied    = true;
    if (payload_len > 0)
        memcpy(slot->payload, payload, payload_len);
    return 0;
}

void reorder_slot_clear(reorder_slot_t *slot) {
    if (slot) memset(slot, 0, sizeof(*slot));
}
