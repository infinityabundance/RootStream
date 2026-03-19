/*
 * abr_ladder.h — Bitrate ladder (quality level definitions)
 *
 * Defines the set of bitrate/resolution/quality tiers available for
 * adaptive bitrate control.  The ladder is static (defined at creation
 * time) and sorted in ascending order of bitrate.
 *
 * Supports up to ABR_LADDER_MAX_LEVELS quality levels.
 */

#ifndef ROOTSTREAM_ABR_LADDER_H
#define ROOTSTREAM_ABR_LADDER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ABR_LADDER_MAX_LEVELS 8

/** A single quality level on the bitrate ladder */
typedef struct {
    int width;            /**< Video width in pixels */
    int height;           /**< Video height in pixels */
    int fps;              /**< Target frame rate */
    uint32_t bitrate_bps; /**< Target video bitrate in bps */
    int quality;          /**< Encoder quality hint 0–100 */
    char name[32];        /**< Human-readable level name */
} abr_level_t;

/** Opaque bitrate ladder */
typedef struct abr_ladder_s abr_ladder_t;

/**
 * abr_ladder_create — allocate ladder with @levels and sort by bitrate
 *
 * @param levels  Array of quality level definitions
 * @param n       Number of levels (1 <= n <= ABR_LADDER_MAX_LEVELS)
 * @return        Non-NULL handle, or NULL on error
 */
abr_ladder_t *abr_ladder_create(const abr_level_t *levels, int n);

/**
 * abr_ladder_destroy — free ladder
 *
 * @param ladder  Ladder to destroy
 */
void abr_ladder_destroy(abr_ladder_t *ladder);

/**
 * abr_ladder_count — number of levels
 *
 * @param ladder  Ladder
 * @return        Level count
 */
int abr_ladder_count(const abr_ladder_t *ladder);

/**
 * abr_ladder_get — retrieve level by index (0 = lowest bitrate)
 *
 * @param ladder  Ladder
 * @param idx     Level index
 * @param out     Output level
 * @return        0 on success, -1 on out-of-range
 */
int abr_ladder_get(const abr_ladder_t *ladder, int idx, abr_level_t *out);

/**
 * abr_ladder_select — find the highest level whose bitrate fits in @budget_bps
 *
 * If @budget_bps is below the lowest level's bitrate, returns index 0.
 *
 * @param ladder      Ladder
 * @param budget_bps  Available bandwidth in bps
 * @return            Level index [0, count-1], or -1 on error
 */
int abr_ladder_select(const abr_ladder_t *ladder, double budget_bps);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_ABR_LADDER_H */
