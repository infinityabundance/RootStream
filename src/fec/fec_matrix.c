/*
 * fec_matrix.c — GF(2) XOR parity matrix implementation
 */

#include "fec_matrix.h"

#include <string.h>

int fec_repair_covers(int src_idx, int repair_idx) {
    /* Source src_idx contributes to repair repair_idx if
     * (src_idx % (repair_idx + 2)) == 0                  */
    return (src_idx % (repair_idx + 2)) == 0;
}

int fec_build_repair(const uint8_t *const *sources,
                       int                   k,
                       int                   repair_idx,
                       uint8_t              *out,
                       size_t                pkt_size) {
    if (!sources || !out || k <= 0 || k > FEC_MAX_K ||
        repair_idx < 0 || repair_idx >= FEC_MAX_R ||
        pkt_size == 0 || pkt_size > FEC_MAX_PKT_SIZE)
        return -1;

    memset(out, 0, pkt_size);

    for (int j = 0; j < k; j++) {
        if (!sources[j]) continue;
        if (fec_repair_covers(j, repair_idx)) {
            for (size_t b = 0; b < pkt_size; b++)
                out[b] ^= sources[j][b];
        }
    }
    return 0;
}
