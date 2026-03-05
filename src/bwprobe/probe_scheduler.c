/*
 * probe_scheduler.c — Probe send scheduler implementation
 */

#include "probe_scheduler.h"

#include <stdlib.h>
#include <string.h>

struct probe_scheduler_s {
    uint64_t interval_us;
    int      burst_size;
    uint16_t next_seq;
    uint32_t burst_id;
    int      burst_remaining;       /* packets still to send in current burst */
    uint64_t last_burst_start_us;   /* µs when the current burst started */
    uint64_t next_burst_us;         /* time when next burst may start */
    uint64_t burst_count;
    uint64_t packet_count;
    bool     first;                 /* flag: first ever tick */
};

probe_scheduler_t *probe_scheduler_create(uint64_t interval_us, int burst_size) {
    if (burst_size < 1 || interval_us == 0) return NULL;
    probe_scheduler_t *s = calloc(1, sizeof(*s));
    if (!s) return NULL;
    s->interval_us     = interval_us;
    s->burst_size      = burst_size;
    s->burst_remaining = 0;
    s->first           = true;
    return s;
}

void probe_scheduler_destroy(probe_scheduler_t *s) { free(s); }

uint64_t probe_scheduler_burst_count(const probe_scheduler_t *s) {
    return s ? s->burst_count : 0;
}

uint64_t probe_scheduler_packet_count(const probe_scheduler_t *s) {
    return s ? s->packet_count : 0;
}

int probe_scheduler_set_interval(probe_scheduler_t *s, uint64_t interval_us) {
    if (!s || interval_us == 0) return -1;
    s->interval_us   = interval_us;
    /* Recompute next burst deadline from the last burst start */
    s->next_burst_us = s->last_burst_start_us + interval_us;
    return 0;
}

probe_sched_decision_t probe_scheduler_tick(probe_scheduler_t *s,
                                               uint64_t           now_us,
                                               probe_packet_t    *pkt_out) {
    if (!s || !pkt_out) return PROBE_SCHED_WAIT;

    /* Mid-burst: send remaining packets */
    if (s->burst_remaining > 0) {
        uint32_t burst_seq = (uint32_t)(s->burst_size - s->burst_remaining);
        pkt_out->seq        = s->next_seq++;
        pkt_out->size_hint  = PROBE_PKT_SIZE;
        pkt_out->send_ts_us = now_us;
        pkt_out->burst_id   = s->burst_id;
        pkt_out->burst_seq  = burst_seq;
        s->burst_remaining--;
        s->packet_count++;
        return PROBE_SCHED_SEND;
    }

    /* Start a new burst? */
    if (s->first || now_us >= s->next_burst_us) {
        s->first                = false;
        s->burst_id++;
        s->burst_remaining      = s->burst_size - 1; /* will send 1st packet now */
        s->last_burst_start_us  = now_us;
        s->next_burst_us        = now_us + s->interval_us;
        s->burst_count++;

        pkt_out->seq        = s->next_seq++;
        pkt_out->size_hint  = PROBE_PKT_SIZE;
        pkt_out->send_ts_us = now_us;
        pkt_out->burst_id   = s->burst_id;
        pkt_out->burst_seq  = 0;
        s->packet_count++;
        return PROBE_SCHED_SEND;
    }

    return PROBE_SCHED_WAIT;
}
