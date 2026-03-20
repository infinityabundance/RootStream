/*
 * per_client_abr.c — Per-client adaptive bitrate controller implementation
 *
 * Uses a simple AIMD (Additive Increase / Multiplicative Decrease) scheme
 * similar to TCP congestion control, tuned for live video streaming.
 */

#include "per_client_abr.h"

#include <stdlib.h>
#include <string.h>

#define ABR_INCREASE_KBPS 500    /* Additive increase per stable interval */
#define ABR_DECREASE_FACTOR 0.7f /* Multiplicative decrease on congestion */
#define ABR_MIN_BITRATE_KBPS 200
#define ABR_LOSS_THRESHOLD 0.05f /* 5% loss triggers decrease */
#define ABR_RTT_THRESHOLD_MS 250 /* High RTT triggers decrease */

struct per_client_abr_s {
    uint32_t bitrate_kbps;
    uint32_t max_bitrate_kbps;
    uint32_t min_bitrate_kbps;
    bool need_keyframe;
    uint32_t stable_intervals; /* Consecutive stable intervals */
};

per_client_abr_t *per_client_abr_create(uint32_t initial_bitrate_kbps, uint32_t max_bitrate_kbps) {
    per_client_abr_t *abr = calloc(1, sizeof(*abr));
    if (!abr)
        return NULL;

    abr->bitrate_kbps = initial_bitrate_kbps;
    abr->max_bitrate_kbps = max_bitrate_kbps;
    abr->min_bitrate_kbps = ABR_MIN_BITRATE_KBPS;
    abr->need_keyframe = false;
    abr->stable_intervals = 0;
    return abr;
}

void per_client_abr_destroy(per_client_abr_t *abr) {
    free(abr);
}

abr_decision_t per_client_abr_update(per_client_abr_t *abr, uint32_t rtt_ms, float loss_rate,
                                     uint32_t bw_kbps) {
    abr_decision_t decision = {
        .target_bitrate_kbps = abr->bitrate_kbps,
        .allow_upgrade = false,
        .force_keyframe = false,
    };

    bool congested = (loss_rate > ABR_LOSS_THRESHOLD) || (rtt_ms > ABR_RTT_THRESHOLD_MS);

    if (congested) {
        /* Multiplicative decrease */
        uint32_t new_rate = (uint32_t)(abr->bitrate_kbps * ABR_DECREASE_FACTOR);
        if (new_rate < abr->min_bitrate_kbps)
            new_rate = abr->min_bitrate_kbps;

        if (new_rate < abr->bitrate_kbps) {
            abr->need_keyframe = true;
            decision.force_keyframe = true;
        }
        abr->bitrate_kbps = new_rate;
        abr->stable_intervals = 0;
    } else {
        abr->stable_intervals++;

        /* Additive increase after 2 stable intervals */
        if (abr->stable_intervals >= 2) {
            uint32_t new_rate = abr->bitrate_kbps + ABR_INCREASE_KBPS;
            /* Cap at measured bandwidth and configured maximum */
            if (bw_kbps > 0 && new_rate > bw_kbps * 9 / 10) {
                new_rate = bw_kbps * 9 / 10;
            }
            if (new_rate > abr->max_bitrate_kbps) {
                new_rate = abr->max_bitrate_kbps;
            }
            abr->bitrate_kbps = new_rate;
            decision.allow_upgrade = true;
        }
    }

    decision.target_bitrate_kbps = abr->bitrate_kbps;

    if (abr->need_keyframe) {
        decision.force_keyframe = true;
        abr->need_keyframe = false;
    }

    return decision;
}

uint32_t per_client_abr_get_bitrate(const per_client_abr_t *abr) {
    return abr ? abr->bitrate_kbps : 0;
}

void per_client_abr_force_keyframe(per_client_abr_t *abr) {
    if (abr)
        abr->need_keyframe = true;
}
