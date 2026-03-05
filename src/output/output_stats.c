/*
 * output_stats.c — Output statistics implementation
 */

#include "output_stats.h"

#include <stdlib.h>
#include <string.h>

struct output_stats_s {
    uint64_t bytes_sent;
    uint32_t connect_count;
    uint32_t error_count;
    int      active_count;
};

output_stats_t *output_stats_create(void) {
    return calloc(1, sizeof(output_stats_t));
}

void output_stats_destroy(output_stats_t *st) { free(st); }

void output_stats_reset(output_stats_t *st) {
    if (st) memset(st, 0, sizeof(*st));
}

int output_stats_record_bytes(output_stats_t *st, uint64_t bytes) {
    if (!st) return -1;
    st->bytes_sent += bytes;
    return 0;
}

int output_stats_record_connect(output_stats_t *st) {
    if (!st) return -1;
    st->connect_count++;
    return 0;
}

int output_stats_record_error(output_stats_t *st) {
    if (!st) return -1;
    st->error_count++;
    return 0;
}

int output_stats_set_active(output_stats_t *st, int count) {
    if (!st) return -1;
    st->active_count = count;
    return 0;
}

int output_stats_snapshot(const output_stats_t *st, output_stats_snapshot_t *out) {
    if (!st || !out) return -1;
    out->bytes_sent    = st->bytes_sent;
    out->connect_count = st->connect_count;
    out->error_count   = st->error_count;
    out->active_count  = st->active_count;
    return 0;
}
