/*
 * abr_stats.c — ABR statistics implementation
 */

#include "abr_stats.h"

#include <stdlib.h>
#include <string.h>

struct abr_stats_s {
    abr_stats_snapshot_t snap;
};

abr_stats_t *abr_stats_create(void) {
    return calloc(1, sizeof(abr_stats_t));
}

void abr_stats_destroy(abr_stats_t *st) {
    free(st);
}

void abr_stats_reset(abr_stats_t *st) {
    if (st) memset(&st->snap, 0, sizeof(st->snap));
}

int abr_stats_record(abr_stats_t *st,
                      int          level_idx,
                      int          prev_idx,
                      int          is_stall) {
    if (!st) return -1;
    abr_stats_snapshot_t *s = &st->snap;
    s->total_ticks++;

    if (level_idx > prev_idx) s->upgrade_count++;
    else if (level_idx < prev_idx) s->downgrade_count++;

    if (is_stall) s->stall_ticks++;

    if (level_idx >= 0 && level_idx < ABR_LADDER_MAX_LEVELS)
        s->ticks_per_level[level_idx]++;

    /* Update running average: avg = avg + (level - avg) / total */
    if (s->total_ticks == 1) {
        s->avg_level = (double)level_idx;
    } else {
        s->avg_level += ((double)level_idx - s->avg_level) /
                         (double)s->total_ticks;
    }
    return 0;
}

int abr_stats_snapshot(const abr_stats_t *st, abr_stats_snapshot_t *out) {
    if (!st || !out) return -1;
    *out = st->snap;
    return 0;
}
