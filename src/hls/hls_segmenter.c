/*
 * hls_segmenter.c — HLS segment lifecycle manager implementation
 */

#include "hls_segmenter.h"
#include "ts_writer.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_SEGS 512

struct hls_segmenter_s {
    hls_segmenter_config_t cfg;
    hls_segment_t          segs[MAX_SEGS];
    int                    seg_count;
    int                    seg_index;        /* current open segment index */
    ts_writer_t           *ts;
    int                    ts_fd;
    bool                   segment_open;
};

hls_segmenter_t *hls_segmenter_create(const hls_segmenter_config_t *cfg) {
    if (!cfg) return NULL;
    hls_segmenter_t *s = calloc(1, sizeof(*s));
    if (!s) return NULL;
    s->cfg    = *cfg;
    s->ts_fd  = -1;
    if (s->cfg.target_duration_s <= 0)
        s->cfg.target_duration_s = HLS_DEFAULT_SEGMENT_DURATION_S;
    if (s->cfg.window_size <= 0)
        s->cfg.window_size = HLS_DEFAULT_WINDOW_SEGMENTS;
    return s;
}

void hls_segmenter_destroy(hls_segmenter_t *seg) {
    if (!seg) return;
    if (seg->ts_fd >= 0) { close(seg->ts_fd); seg->ts_fd = -1; }
    ts_writer_destroy(seg->ts);
    free(seg);
}

int hls_segmenter_open_segment(hls_segmenter_t *seg) {
    if (!seg || seg->segment_open) return -1;

    /* Build path: dir/base_nameINDEX.ts — total bounded by HLS_MAX_PATH+SEG */
    char path[HLS_MAX_PATH + HLS_MAX_SEG_NAME * 2 + 4];
    snprintf(path, sizeof(path), "%s/%.*s%d.ts",
             seg->cfg.output_dir,
             (int)(HLS_MAX_SEG_NAME - 14),  /* leave room for index + ".ts" */
             seg->cfg.base_name,
             seg->seg_index);

    seg->ts_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (seg->ts_fd < 0) return -1;

    ts_writer_destroy(seg->ts);
    seg->ts = ts_writer_create(seg->ts_fd);
    if (!seg->ts) { close(seg->ts_fd); seg->ts_fd = -1; return -1; }

    if (ts_writer_write_pat_pmt(seg->ts) != 0) return -1;

    seg->segment_open = true;
    return 0;
}

int hls_segmenter_write(hls_segmenter_t *seg,
                          const uint8_t   *data,
                          size_t           len,
                          uint64_t         pts_90khz,
                          bool             is_kf) {
    if (!seg || !seg->segment_open || !seg->ts) return -1;
    return ts_writer_write_pes(seg->ts, data, len, pts_90khz, is_kf);
}

int hls_segmenter_close_segment(hls_segmenter_t *seg, double duration_s) {
    if (!seg || !seg->segment_open) return -1;

    close(seg->ts_fd); seg->ts_fd = -1;
    ts_writer_destroy(seg->ts); seg->ts = NULL;
    seg->segment_open = false;

    if (seg->seg_count < MAX_SEGS) {
        hls_segment_t *s = &seg->segs[seg->seg_count];
        snprintf(s->filename, HLS_MAX_SEG_NAME, "%.*s%d.ts",
                 (int)(HLS_MAX_SEG_NAME - 14), seg->cfg.base_name,
                 seg->seg_index);
        s->duration_s      = duration_s;
        s->is_discontinuity = false;
        seg->seg_count++;
    }
    seg->seg_index++;
    return 0;
}

int hls_segmenter_update_manifest(hls_segmenter_t *seg) {
    if (!seg || seg->seg_count == 0) return -1;

    char path[HLS_MAX_PATH + HLS_MAX_SEG_NAME + 2];
    char tmp[HLS_MAX_PATH + HLS_MAX_SEG_NAME + 8];
    snprintf(path, sizeof(path), "%s/%s",
             seg->cfg.output_dir, seg->cfg.playlist_name);
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    char buf[65536];
    int n;
    if (seg->cfg.vod_mode) {
        n = m3u8_write_vod(seg->segs, seg->seg_count,
                            seg->cfg.target_duration_s, buf, sizeof(buf));
    } else {
        n = m3u8_write_live(seg->segs, seg->seg_count,
                             seg->cfg.window_size,
                             seg->cfg.target_duration_s,
                             0, buf, sizeof(buf));
    }
    if (n < 0) return -1;

    FILE *f = fopen(tmp, "w");
    if (!f) return -1;
    if (fwrite(buf, 1, (size_t)n, f) != (size_t)n) {
        fclose(f); remove(tmp); return -1;
    }
    fclose(f);
    return rename(tmp, path) == 0 ? 0 : -1;
}

size_t hls_segmenter_segment_count(const hls_segmenter_t *seg) {
    return seg ? (size_t)seg->seg_count : 0;
}
