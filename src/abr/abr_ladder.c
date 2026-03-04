/*
 * abr_ladder.c — Bitrate ladder implementation
 */

#include "abr_ladder.h"

#include <stdlib.h>
#include <string.h>

struct abr_ladder_s {
    abr_level_t levels[ABR_LADDER_MAX_LEVELS];
    int         count;
};

static int cmp_levels(const void *a, const void *b) {
    const abr_level_t *la = (const abr_level_t *)a;
    const abr_level_t *lb = (const abr_level_t *)b;
    if (la->bitrate_bps < lb->bitrate_bps) return -1;
    if (la->bitrate_bps > lb->bitrate_bps) return  1;
    return 0;
}

abr_ladder_t *abr_ladder_create(const abr_level_t *levels, int n) {
    if (!levels || n < 1 || n > ABR_LADDER_MAX_LEVELS) return NULL;
    abr_ladder_t *l = malloc(sizeof(*l));
    if (!l) return NULL;
    l->count = n;
    memcpy(l->levels, levels, (size_t)n * sizeof(abr_level_t));
    qsort(l->levels, (size_t)n, sizeof(abr_level_t), cmp_levels);
    return l;
}

void abr_ladder_destroy(abr_ladder_t *ladder) {
    free(ladder);
}

int abr_ladder_count(const abr_ladder_t *ladder) {
    return ladder ? ladder->count : 0;
}

int abr_ladder_get(const abr_ladder_t *ladder, int idx, abr_level_t *out) {
    if (!ladder || !out || idx < 0 || idx >= ladder->count) return -1;
    *out = ladder->levels[idx];
    return 0;
}

int abr_ladder_select(const abr_ladder_t *ladder, double budget_bps) {
    if (!ladder || ladder->count == 0) return -1;
    /* Return index of highest level that fits in budget */
    int best = 0;
    for (int i = 0; i < ladder->count; i++) {
        if ((double)ladder->levels[i].bitrate_bps <= budget_bps)
            best = i;
        else
            break;
    }
    return best;
}
