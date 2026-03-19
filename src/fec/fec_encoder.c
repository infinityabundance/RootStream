/*
 * fec_encoder.c — FEC group encoder
 */

#include "fec_encoder.h"

#include <string.h>

int fec_encode(const uint8_t *const *sources, int k, int r, uint8_t **out, size_t pkt_size) {
    if (!sources || !out || k <= 0 || k > FEC_MAX_K || r < 0 || r > FEC_MAX_R || pkt_size == 0 ||
        pkt_size > FEC_MAX_PKT_SIZE)
        return -1;

    /* Copy source packets into output positions 0..k-1 */
    for (int i = 0; i < k; i++) {
        if (!sources[i] || !out[i])
            return -1;
        memcpy(out[i], sources[i], pkt_size);
    }

    /* Compute repair packets into output positions k..k+r-1 */
    for (int ri = 0; ri < r; ri++) {
        if (!out[k + ri])
            return -1;
        int rc = fec_build_repair(sources, k, ri, out[k + ri], pkt_size);
        if (rc < 0)
            return -1;
    }
    return 0;
}
