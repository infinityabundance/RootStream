/*
 * ladder_selector.h — ABR rung selector
 *
 * Given a bitrate ladder (ascending) and an estimated available
 * bandwidth, selects the highest rung whose bitrate fits within the
 * bandwidth budget (with optional headroom margin).
 *
 * Thread-safety: stateless function — thread-safe.
 */

#ifndef ROOTSTREAM_LADDER_SELECTOR_H
#define ROOTSTREAM_LADDER_SELECTOR_H

#include "ladder_rung.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ladder_select — pick the best rung fitting estimated bandwidth
 *
 * Iterates rungs in ascending bitrate order and returns the last rung
 * whose bitrate ≤ (estimated_bw_bps × (1 − margin)).
 *
 * @param rungs         Ascending bitrate array
 * @param n             Number of rungs
 * @param estimated_bw  Estimated available bandwidth (bits/sec)
 * @param margin        Safety margin in [0, 1) (e.g. 0.2 = 20% headroom)
 * @return              Index of selected rung (0..n-1), or 0 if none fit
 */
int ladder_select(const ladder_rung_t *rungs,
                    int                  n,
                    uint32_t             estimated_bw,
                    float                margin);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_LADDER_SELECTOR_H */
