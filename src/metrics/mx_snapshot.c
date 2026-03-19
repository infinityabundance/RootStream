/*
 * mx_snapshot.c — Timestamped metrics snapshot
 */

#include "mx_snapshot.h"

#include <string.h>

int mx_snapshot_init(mx_snapshot_t *s) {
    if (!s)
        return -1;
    memset(s, 0, sizeof(*s));
    return 0;
}

int mx_snapshot_dump(const mx_snapshot_t *s, mx_gauge_t *out, int max_out) {
    if (!s || !out || max_out <= 0)
        return -1;
    int n = (s->gauge_count < max_out) ? s->gauge_count : max_out;
    for (int i = 0; i < n; i++) out[i] = s->gauges[i];
    return n;
}
