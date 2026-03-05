/*
 * congestion_stats.c — RTT + loss congestion stats aggregator
 */

#include "congestion_stats.h"

#include <stdlib.h>
#include <string.h>

struct congestion_stats_s {
    rtt_estimator_t  *rtt;
    loss_detector_t  *loss;
    bool              was_congested;
    uint64_t          congestion_events;
    uint64_t          recovery_events;
};

congestion_stats_t *congestion_stats_create(double loss_threshold) {
    congestion_stats_t *cs = calloc(1, sizeof(*cs));
    if (!cs) return NULL;
    cs->rtt = rtt_estimator_create();
    cs->loss = loss_detector_create(loss_threshold);
    if (!cs->rtt || !cs->loss) {
        rtt_estimator_destroy(cs->rtt);
        loss_detector_destroy(cs->loss);
        free(cs);
        return NULL;
    }
    return cs;
}

void congestion_stats_destroy(congestion_stats_t *cs) {
    if (!cs) return;
    rtt_estimator_destroy(cs->rtt);
    loss_detector_destroy(cs->loss);
    free(cs);
}

void congestion_stats_reset(congestion_stats_t *cs) {
    if (!cs) return;
    rtt_estimator_reset(cs->rtt);
    loss_detector_reset(cs->loss);
    cs->was_congested     = false;
    cs->congestion_events = 0;
    cs->recovery_events   = 0;
}

int congestion_stats_record_rtt(congestion_stats_t *cs, uint64_t rtt_us) {
    if (!cs) return -1;
    return rtt_estimator_update(cs->rtt, rtt_us);
}

loss_signal_t congestion_stats_record_packet(congestion_stats_t *cs,
                                               loss_outcome_t      outcome) {
    if (!cs) return LOSS_SIGNAL_NONE;
    loss_signal_t sig = loss_detector_record(cs->loss, outcome);
    bool now_congested = (sig == LOSS_SIGNAL_CONGESTED);

    if (now_congested && !cs->was_congested)
        cs->congestion_events++;
    else if (!now_congested && cs->was_congested)
        cs->recovery_events++;
    cs->was_congested = now_congested;
    return sig;
}

int congestion_stats_snapshot(const congestion_stats_t *cs,
                                congestion_snapshot_t    *out) {
    if (!cs || !out) return -1;
    rtt_estimator_snapshot(cs->rtt, &out->rtt);
    out->loss_fraction      = loss_detector_loss_fraction(cs->loss);
    out->congested          = loss_detector_is_congested(cs->loss);
    out->congestion_events  = cs->congestion_events;
    out->recovery_events    = cs->recovery_events;
    return 0;
}
