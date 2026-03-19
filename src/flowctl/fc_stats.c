/*
 * fc_stats.c — Flow controller statistics
 */

#include "fc_stats.h"

#include <stdlib.h>
#include <string.h>

struct fc_stats_s {
    uint64_t bytes_sent;
    uint64_t bytes_dropped;
    uint64_t stalls;
    uint64_t replenish_count;
};

fc_stats_t *fc_stats_create(void) {
    return calloc(1, sizeof(fc_stats_t));
}
void fc_stats_destroy(fc_stats_t *st) {
    free(st);
}
void fc_stats_reset(fc_stats_t *st) {
    if (st)
        memset(st, 0, sizeof(*st));
}

int fc_stats_record_send(fc_stats_t *st, uint32_t bytes) {
    if (!st)
        return -1;
    st->bytes_sent += bytes;
    return 0;
}
int fc_stats_record_drop(fc_stats_t *st, uint32_t bytes) {
    if (!st)
        return -1;
    st->bytes_dropped += bytes;
    return 0;
}
int fc_stats_record_stall(fc_stats_t *st) {
    if (!st)
        return -1;
    st->stalls++;
    return 0;
}
int fc_stats_record_replenish(fc_stats_t *st) {
    if (!st)
        return -1;
    st->replenish_count++;
    return 0;
}

int fc_stats_snapshot(const fc_stats_t *st, fc_stats_snapshot_t *out) {
    if (!st || !out)
        return -1;
    out->bytes_sent = st->bytes_sent;
    out->bytes_dropped = st->bytes_dropped;
    out->stalls = st->stalls;
    out->replenish_count = st->replenish_count;
    return 0;
}
