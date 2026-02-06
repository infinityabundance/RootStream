/*
 * packet_validate.c - Lightweight packet validation
 */

#include "../include/rootstream.h"

int rootstream_net_validate_packet(const uint8_t *buffer, size_t len) {
    if (!buffer || len < sizeof(packet_header_t)) {
        return -1;
    }

    const packet_header_t *hdr = (const packet_header_t*)buffer;
    if (hdr->magic != 0x524F4F54) {
        return -1;
    }

    if (hdr->version != 1) {
        return -1;
    }

    if (hdr->payload_size > len - sizeof(packet_header_t)) {
        return -1;
    }

    return 0;
}
