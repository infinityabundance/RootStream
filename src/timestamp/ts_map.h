/*
 * ts_map.h — Linear PTS ↔ wall-clock mapper
 *
 * Maps a stream presentation timestamp (PTS, in ticks of a known
 * timebase) to a wall-clock microsecond value using an anchor point
 * and a slope derived from the timebase.
 *
 *   wall_us = anchor_us + (pts - anchor_pts) * us_per_tick
 *
 * The anchor is updated via `ts_map_set_anchor()` when a reliable
 * reference measurement arrives (e.g. an NTP-corrected timestamp).
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_TS_MAP_H
#define ROOTSTREAM_TS_MAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** PTS ↔ wall-clock linear mapper */
typedef struct {
    int64_t anchor_pts; /**< Reference PTS at last anchor update */
    int64_t anchor_us;  /**< Wall-clock µs at last anchor update */
    double us_per_tick; /**< Conversion: µs per PTS tick */
    int initialised;
} ts_map_t;

/**
 * ts_map_init — initialise mapper
 *
 * @param m             Mapper to initialise
 * @param timebase_num  Timebase numerator   (e.g. 1)
 * @param timebase_den  Timebase denominator (e.g. 90000 for 90 kHz)
 * @return              0 on success, -1 on NULL or den=0
 */
int ts_map_init(ts_map_t *m, int timebase_num, int timebase_den);

/**
 * ts_map_set_anchor — set new reference point
 *
 * @param m        Mapper
 * @param pts      Stream PTS at the anchor
 * @param wall_us  Wall-clock µs at the anchor
 * @return         0 on success, -1 on NULL
 */
int ts_map_set_anchor(ts_map_t *m, int64_t pts, int64_t wall_us);

/**
 * ts_map_pts_to_us — convert PTS → wall-clock µs
 *
 * @param m    Mapper
 * @param pts  Stream PTS
 * @return     Wall-clock µs, or 0 if uninitialised
 */
int64_t ts_map_pts_to_us(const ts_map_t *m, int64_t pts);

/**
 * ts_map_us_to_pts — convert wall-clock µs → PTS
 *
 * @param m       Mapper
 * @param wall_us Wall-clock µs
 * @return        Stream PTS, or 0 if uninitialised
 */
int64_t ts_map_us_to_pts(const ts_map_t *m, int64_t wall_us);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_TS_MAP_H */
