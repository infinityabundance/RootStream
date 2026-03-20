/*
 * pq_stats.c — Priority queue statistics implementation
 */

#include "pq_stats.h"

#include <stdlib.h>
#include <string.h>

struct pq_stats_s {
    uint64_t push_count;
    uint64_t pop_count;
    int peak_size;
    uint64_t overflow_count;
};

pq_stats_t *pq_stats_create(void) {
    return calloc(1, sizeof(pq_stats_t));
}

void pq_stats_destroy(pq_stats_t *st) {
    free(st);
}

void pq_stats_reset(pq_stats_t *st) {
    if (st)
        memset(st, 0, sizeof(*st));
}

int pq_stats_record_push(pq_stats_t *st, int cur_size) {
    if (!st)
        return -1;
    st->push_count++;
    if (cur_size > st->peak_size)
        st->peak_size = cur_size;
    return 0;
}

int pq_stats_record_pop(pq_stats_t *st) {
    if (!st)
        return -1;
    st->pop_count++;
    return 0;
}

int pq_stats_record_overflow(pq_stats_t *st) {
    if (!st)
        return -1;
    st->overflow_count++;
    return 0;
}

int pq_stats_snapshot(const pq_stats_t *st, pq_stats_snapshot_t *out) {
    if (!st || !out)
        return -1;
    out->push_count = st->push_count;
    out->pop_count = st->pop_count;
    out->peak_size = st->peak_size;
    out->overflow_count = st->overflow_count;
    return 0;
}
