/*
 * hls_segmenter.h — HLS segment lifecycle manager
 *
 * Manages the on-disk lifecycle of HLS segments: creates new segment
 * files, tracks open/complete status, enforces the sliding window by
 * deleting old segments, and updates the M3U8 manifest atomically.
 *
 * Typical usage
 * ─────────────
 *   hls_segmenter_t *seg = hls_segmenter_create(&cfg);
 *   hls_segmenter_open_segment(seg);        // start new segment
 *   hls_segmenter_write(seg, nal, len, pts, is_kf);  // write frames
 *   hls_segmenter_close_segment(seg, dur);  // finish segment
 *   hls_segmenter_update_manifest(seg);     // rewrite M3U8
 *   hls_segmenter_destroy(seg);
 */

#ifndef ROOTSTREAM_HLS_SEGMENTER_H
#define ROOTSTREAM_HLS_SEGMENTER_H

#include "hls_config.h"
#include "m3u8_writer.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** HLS segmenter configuration */
typedef struct {
    char   output_dir[HLS_MAX_PATH];    /**< Directory for .ts and .m3u8 files */
    char   base_name[HLS_MAX_SEG_NAME]; /**< Segment base name (e.g. "seg") */
    char   playlist_name[HLS_MAX_SEG_NAME]; /**< Playlist file name */
    int    target_duration_s;           /**< Target segment duration */
    int    window_size;                 /**< Sliding window (live) */
    bool   vod_mode;                    /**< Write VOD playlist */
} hls_segmenter_config_t;

/** Opaque HLS segmenter */
typedef struct hls_segmenter_s hls_segmenter_t;

/**
 * hls_segmenter_create — allocate segmenter with @cfg
 *
 * @param cfg  Configuration
 * @return     Non-NULL handle, or NULL on OOM / bad config
 */
hls_segmenter_t *hls_segmenter_create(const hls_segmenter_config_t *cfg);

/**
 * hls_segmenter_destroy — free segmenter (does not delete files)
 *
 * @param seg  Segmenter to destroy
 */
void hls_segmenter_destroy(hls_segmenter_t *seg);

/**
 * hls_segmenter_open_segment — begin a new segment file
 *
 * @param seg  Segmenter
 * @return     0 on success, -1 on error
 */
int hls_segmenter_open_segment(hls_segmenter_t *seg);

/**
 * hls_segmenter_write — write a NAL frame to the current open segment
 *
 * @param seg        Segmenter
 * @param data       NAL unit data
 * @param len        Data length in bytes
 * @param pts_90khz  Presentation timestamp in 90 kHz units
 * @param is_kf      True if IDR / keyframe
 * @return           0 on success, -1 on error
 */
int hls_segmenter_write(hls_segmenter_t *seg,
                          const uint8_t   *data,
                          size_t           len,
                          uint64_t         pts_90khz,
                          bool             is_kf);

/**
 * hls_segmenter_close_segment — finalise the current segment
 *
 * @param seg         Segmenter
 * @param duration_s  Actual segment duration in seconds
 * @return            0 on success, -1 on error
 */
int hls_segmenter_close_segment(hls_segmenter_t *seg, double duration_s);

/**
 * hls_segmenter_update_manifest — rewrite the M3U8 playlist file
 *
 * @param seg  Segmenter
 * @return     0 on success, -1 on error
 */
int hls_segmenter_update_manifest(hls_segmenter_t *seg);

/**
 * hls_segmenter_segment_count — total completed segments so far
 *
 * @param seg  Segmenter
 * @return     Count
 */
size_t hls_segmenter_segment_count(const hls_segmenter_t *seg);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_HLS_SEGMENTER_H */
