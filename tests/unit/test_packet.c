#include "../../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fill_random(uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)(rand() & 0xFF);
    }
}

int main(void) {
    srand(1);

    uint8_t buffer[512];

    /* Random fuzz cases should never crash */
    for (int i = 0; i < 1000; i++) {
        size_t len = (size_t)(rand() % sizeof(buffer));
        fill_random(buffer, len);
        (void)rootstream_net_validate_packet(buffer, len);
    }

    /* Construct a minimal valid packet */
    packet_header_t hdr = {0};
    hdr.magic = 0x524F4F54;
    hdr.version = 1;
    hdr.type = PKT_PING;
    hdr.payload_size = 0;

    memcpy(buffer, &hdr, sizeof(hdr));
    if (rootstream_net_validate_packet(buffer, sizeof(hdr)) != 0) {
        fprintf(stderr, "Expected valid packet to pass validation\n");
        return 1;
    }

    /* Invalid payload size should fail */
    hdr.payload_size = 1024;
    memcpy(buffer, &hdr, sizeof(hdr));
    if (rootstream_net_validate_packet(buffer, sizeof(hdr)) == 0) {
        fprintf(stderr, "Expected oversized payload to fail validation\n");
        return 1;
    }

    return 0;
}
