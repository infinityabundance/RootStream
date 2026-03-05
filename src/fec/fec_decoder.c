/*
 * fec_decoder.c — FEC XOR recovery decoder
 */

#include "fec_decoder.h"

#include <string.h>
#include <stdlib.h>

int fec_decode(const uint8_t *const *pkts,
                 const bool           *received,
                 int                   k,
                 int                   r,
                 size_t                pkt_size,
                 uint8_t             **recovered) {
    if (!pkts || !received || !recovered || k <= 0 || k > FEC_MAX_K ||
        r < 0 || r > FEC_MAX_R ||
        pkt_size == 0 || pkt_size > FEC_MAX_PKT_SIZE)
        return -1;

    int n_recovered = 0;

    /* For each repair block ri, check if exactly one source covered by it
     * is missing.  If so, recover it. */
    for (int ri = 0; ri < r; ri++) {
        int repair_idx = k + ri;
        /* Only use this repair if the repair packet was received */
        if (!received[repair_idx] || !pkts[repair_idx]) continue;

        /* Count missing sources covered by this repair */
        int missing_idx = -1;
        int missing_count = 0;
        for (int j = 0; j < k; j++) {
            if (fec_repair_covers(j, ri) && !received[j]) {
                missing_count++;
                missing_idx = j;
            }
        }
        if (missing_count != 1 || missing_idx < 0) continue;
        if (!recovered[missing_idx]) continue;

        /* recovered[missing_idx] = repair XOR all other present sources */
        memcpy(recovered[missing_idx], pkts[repair_idx], pkt_size);
        for (int j = 0; j < k; j++) {
            if (!fec_repair_covers(j, ri)) continue;
            if (j == missing_idx) continue;
            if (!received[j] || !pkts[j]) continue;
            for (size_t b = 0; b < pkt_size; b++)
                recovered[missing_idx][b] ^= pkts[j][b];
        }
        n_recovered++;
    }
    return n_recovered;
}
