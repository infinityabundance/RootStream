/*
 * m3u8_writer.c — M3U8 playlist generator implementation
 */

#include "m3u8_writer.h"

#include <stdio.h>
#include <string.h>

/* ── Live playlist ────────────────────────────────────────────────── */

int m3u8_write_live(const hls_segment_t *segments, int n, int window_size, int target_dur_s,
                    int media_seq, char *buf, size_t buf_sz) {
    if (!segments || !buf || buf_sz == 0)
        return -1;
    if (n <= 0 || window_size <= 0)
        return -1;

    /* Window: last @window_size segments */
    int start = n - window_size;
    if (start < 0)
        start = 0;
    int window_n = n - start;

    /* Sequence number is base + how many segments we skipped */
    int seq = media_seq + start;

    size_t pos = 0;
    int r;

    r = snprintf(buf + pos, buf_sz - pos,
                 "#EXTM3U\n"
                 "#EXT-X-VERSION:3\n"
                 "#EXT-X-TARGETDURATION:%d\n"
                 "#EXT-X-MEDIA-SEQUENCE:%d\n",
                 target_dur_s, seq);
    if (r < 0 || (size_t)r >= buf_sz - pos)
        return -1;
    pos += (size_t)r;

    for (int i = start; i < start + window_n; i++) {
        if (segments[i].is_discontinuity) {
            r = snprintf(buf + pos, buf_sz - pos, "#EXT-X-DISCONTINUITY\n");
            if (r < 0 || (size_t)r >= buf_sz - pos)
                return -1;
            pos += (size_t)r;
        }
        r = snprintf(buf + pos, buf_sz - pos, "#EXTINF:%.6f,\n%s\n", segments[i].duration_s,
                     segments[i].filename);
        if (r < 0 || (size_t)r >= buf_sz - pos)
            return -1;
        pos += (size_t)r;
    }

    return (int)pos;
}

/* ── VOD playlist ─────────────────────────────────────────────────── */

int m3u8_write_vod(const hls_segment_t *segments, int n, int target_dur_s, char *buf,
                   size_t buf_sz) {
    if (!segments || !buf || buf_sz == 0 || n <= 0)
        return -1;

    size_t pos = 0;
    int r;

    r = snprintf(buf + pos, buf_sz - pos,
                 "#EXTM3U\n"
                 "#EXT-X-VERSION:3\n"
                 "#EXT-X-TARGETDURATION:%d\n"
                 "#EXT-X-PLAYLIST-TYPE:VOD\n"
                 "#EXT-X-MEDIA-SEQUENCE:0\n",
                 target_dur_s);
    if (r < 0 || (size_t)r >= buf_sz - pos)
        return -1;
    pos += (size_t)r;

    for (int i = 0; i < n; i++) {
        if (segments[i].is_discontinuity) {
            r = snprintf(buf + pos, buf_sz - pos, "#EXT-X-DISCONTINUITY\n");
            if (r < 0 || (size_t)r >= buf_sz - pos)
                return -1;
            pos += (size_t)r;
        }
        r = snprintf(buf + pos, buf_sz - pos, "#EXTINF:%.6f,\n%s\n", segments[i].duration_s,
                     segments[i].filename);
        if (r < 0 || (size_t)r >= buf_sz - pos)
            return -1;
        pos += (size_t)r;
    }

    r = snprintf(buf + pos, buf_sz - pos, "#EXT-X-ENDLIST\n");
    if (r < 0 || (size_t)r >= buf_sz - pos)
        return -1;
    pos += (size_t)r;

    return (int)pos;
}

/* ── Master playlist ─────────────────────────────────────────────── */

int m3u8_write_master(const char **uris, const int *bandwidths, int n, int width, int height,
                      char *buf, size_t buf_sz) {
    if (!uris || !bandwidths || !buf || buf_sz == 0 || n <= 0)
        return -1;

    size_t pos = 0;
    int r;

    r = snprintf(buf + pos, buf_sz - pos, "#EXTM3U\n#EXT-X-VERSION:3\n");
    if (r < 0 || (size_t)r >= buf_sz - pos)
        return -1;
    pos += (size_t)r;

    for (int i = 0; i < n; i++) {
        if (width > 0 && height > 0) {
            r = snprintf(buf + pos, buf_sz - pos,
                         "#EXT-X-STREAM-INF:BANDWIDTH=%d,RESOLUTION=%dx%d\n%s\n", bandwidths[i],
                         width, height, uris[i]);
        } else {
            r = snprintf(buf + pos, buf_sz - pos, "#EXT-X-STREAM-INF:BANDWIDTH=%d\n%s\n",
                         bandwidths[i], uris[i]);
        }
        if (r < 0 || (size_t)r >= buf_sz - pos)
            return -1;
        pos += (size_t)r;
    }

    return (int)pos;
}
