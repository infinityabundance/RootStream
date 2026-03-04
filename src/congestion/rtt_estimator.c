/*
 * rtt_estimator.c — RFC 6298 RTT/SRTT/RTTVAR/RTO estimator
 */

#include "rtt_estimator.h"

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

struct rtt_estimator_s {
    double   srtt_us;
    double   rttvar_us;
    uint64_t sample_count;
    double   min_rtt_us;
    double   max_rtt_us;
};

rtt_estimator_t *rtt_estimator_create(void) {
    rtt_estimator_t *e = calloc(1, sizeof(*e));
    if (e) e->min_rtt_us = DBL_MAX;
    return e;
}

void rtt_estimator_destroy(rtt_estimator_t *e) { free(e); }

void rtt_estimator_reset(rtt_estimator_t *e) {
    if (e) { memset(e, 0, sizeof(*e)); e->min_rtt_us = DBL_MAX; }
}

bool rtt_estimator_has_samples(const rtt_estimator_t *e) {
    return e && e->sample_count > 0;
}

int rtt_estimator_update(rtt_estimator_t *e, uint64_t rtt_us) {
    if (!e || rtt_us == 0) return -1;
    double r = (double)rtt_us;

    if (e->sample_count == 0) {
        /* RFC 6298 §2.2 first-sample initialisation */
        e->srtt_us   = r;
        e->rttvar_us = r / 2.0;
    } else {
        /* α = 1/8, β = 1/4 */
        double diff = fabs(e->srtt_us - r);
        e->rttvar_us = 0.75 * e->rttvar_us + 0.25 * diff;
        e->srtt_us   = 0.875 * e->srtt_us + 0.125 * r;
    }

    e->sample_count++;
    if (r < e->min_rtt_us) e->min_rtt_us = r;
    if (r > e->max_rtt_us) e->max_rtt_us = r;
    return 0;
}

int rtt_estimator_snapshot(const rtt_estimator_t *e, rtt_snapshot_t *out) {
    if (!e || !out) return -1;
    out->srtt_us      = e->srtt_us;
    out->rttvar_us    = e->rttvar_us;
    /* RTO = SRTT + max(G, K*RTTVAR) */
    double k_rttvar = RTT_K * e->rttvar_us;
    double g        = (double)RTT_CLOCK_GRANULARITY_US;
    out->rto_us       = e->srtt_us + (k_rttvar > g ? k_rttvar : g);
    out->min_rtt_us   = (e->min_rtt_us == DBL_MAX) ? 0.0 : e->min_rtt_us;
    out->max_rtt_us   = e->max_rtt_us;
    out->sample_count = e->sample_count;
    return 0;
}
