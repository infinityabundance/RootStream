/*
 * hr_stats.c — Hot-reload statistics implementation
 */

#include "hr_stats.h"

#include <stdlib.h>
#include <string.h>

struct hr_stats_s {
    uint64_t reload_count;
    uint64_t fail_count;
    uint64_t last_reload_us;
    int      loaded_plugins;
};

hr_stats_t *hr_stats_create(void) {
    return calloc(1, sizeof(hr_stats_t));
}

void hr_stats_destroy(hr_stats_t *st) { free(st); }

void hr_stats_reset(hr_stats_t *st) {
    if (st) memset(st, 0, sizeof(*st));
}

int hr_stats_record_reload(hr_stats_t *st, int success, uint64_t now_us) {
    if (!st) return -1;
    if (success) {
        st->reload_count++;
        st->last_reload_us = now_us;
    } else {
        st->fail_count++;
    }
    return 0;
}

int hr_stats_set_loaded(hr_stats_t *st, int count) {
    if (!st) return -1;
    st->loaded_plugins = count;
    return 0;
}

int hr_stats_snapshot(const hr_stats_t *st, hr_stats_snapshot_t *out) {
    if (!st || !out) return -1;
    out->reload_count   = st->reload_count;
    out->fail_count     = st->fail_count;
    out->last_reload_us = st->last_reload_us;
    out->loaded_plugins = st->loaded_plugins;
    return 0;
}
