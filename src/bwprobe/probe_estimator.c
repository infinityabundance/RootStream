/*
 * probe_estimator.c — One-way delay and bandwidth estimator
 */

#include "probe_estimator.h"

#include <float.h>
#include <stdlib.h>
#include <string.h>

struct probe_estimator_s {
    double owd_us; /* smoothed OWD */
    double owd_min_us;
    double owd_max_us;
    /* bandwidth: running average of bytes_per_us, converted to bps */
    double bw_bps;
    uint64_t sample_count;
    /* previous recv_ts for inter-arrival gap bandwidth calc */
    uint64_t prev_recv_us;
    uint64_t prev_size;
};

probe_estimator_t *probe_estimator_create(void) {
    probe_estimator_t *pe = calloc(1, sizeof(*pe));
    if (pe)
        pe->owd_min_us = DBL_MAX;
    return pe;
}

void probe_estimator_destroy(probe_estimator_t *pe) {
    free(pe);
}

void probe_estimator_reset(probe_estimator_t *pe) {
    if (!pe)
        return;
    memset(pe, 0, sizeof(*pe));
    pe->owd_min_us = DBL_MAX;
}

bool probe_estimator_has_samples(const probe_estimator_t *pe) {
    return pe && pe->sample_count > 0;
}

int probe_estimator_observe(probe_estimator_t *pe, uint64_t send_ts_us, uint64_t recv_ts_us,
                            uint32_t size_bytes) {
    if (!pe || recv_ts_us < send_ts_us)
        return -1;

    double owd = (double)(recv_ts_us - send_ts_us);

    if (pe->sample_count == 0) {
        pe->owd_us = owd;
    } else {
        pe->owd_us = (1.0 - PROBE_OWD_ALPHA) * pe->owd_us + PROBE_OWD_ALPHA * owd;
    }
    if (owd < pe->owd_min_us)
        pe->owd_min_us = owd;
    if (owd > pe->owd_max_us)
        pe->owd_max_us = owd;

    /* Bandwidth estimate: bits transferred / inter-arrival gap */
    if (pe->prev_recv_us > 0 && recv_ts_us > pe->prev_recv_us && size_bytes > 0) {
        double gap_us = (double)(recv_ts_us - pe->prev_recv_us);
        double bw_inst = (double)size_bytes * 8.0 / (gap_us / 1e6); /* bits/s */
        /* EWMA with same α */
        if (pe->bw_bps == 0.0)
            pe->bw_bps = bw_inst;
        else
            pe->bw_bps = (1.0 - PROBE_OWD_ALPHA) * pe->bw_bps + PROBE_OWD_ALPHA * bw_inst;
    }

    pe->prev_recv_us = recv_ts_us;
    pe->prev_size = size_bytes;
    pe->sample_count++;
    return 0;
}

int probe_estimator_snapshot(const probe_estimator_t *pe, probe_estimate_t *out) {
    if (!pe || !out)
        return -1;
    out->owd_us = pe->owd_us;
    out->owd_min_us = (pe->owd_min_us == DBL_MAX) ? 0.0 : pe->owd_min_us;
    out->owd_max_us = pe->owd_max_us;
    out->bw_bps = pe->bw_bps;
    out->sample_count = pe->sample_count;
    return 0;
}
