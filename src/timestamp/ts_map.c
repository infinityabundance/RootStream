/*
 * ts_map.c — Linear PTS ↔ wall-clock mapper
 */

#include "ts_map.h"
#include <string.h>

int ts_map_init(ts_map_t *m, int timebase_num, int timebase_den) {
    if (!m || timebase_den == 0) return -1;
    memset(m, 0, sizeof(*m));
    /* µs per tick = (num/den) × 1e6 */
    m->us_per_tick = ((double)timebase_num / (double)timebase_den) * 1e6;
    return 0;
}

int ts_map_set_anchor(ts_map_t *m, int64_t pts, int64_t wall_us) {
    if (!m) return -1;
    m->anchor_pts  = pts;
    m->anchor_us   = wall_us;
    m->initialised = 1;
    return 0;
}

int64_t ts_map_pts_to_us(const ts_map_t *m, int64_t pts) {
    if (!m || !m->initialised) return 0;
    return m->anchor_us + (int64_t)((pts - m->anchor_pts) * m->us_per_tick);
}

int64_t ts_map_us_to_pts(const ts_map_t *m, int64_t wall_us) {
    if (!m || !m->initialised || m->us_per_tick == 0.0) return 0;
    return m->anchor_pts + (int64_t)((wall_us - m->anchor_us) / m->us_per_tick);
}
