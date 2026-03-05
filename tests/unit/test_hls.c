/*
 * test_hls.c — Unit tests for PHASE-44 HLS Segment Output
 *
 * Tests ts_writer (PAT/PMT/PES generation), m3u8_writer (live/VOD/master
 * manifests), and hls_segmenter (open/write/close/manifest lifecycle).
 * Uses /tmp for file I/O; no video hardware required.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "../../src/hls/hls_config.h"
#include "../../src/hls/ts_writer.h"
#include "../../src/hls/m3u8_writer.h"
#include "../../src/hls/hls_segmenter.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── ts_writer tests ─────────────────────────────────────────────── */

static int test_ts_writer_create(void) {
    printf("\n=== test_ts_writer_create ===\n");

    int fd = open("/tmp/test_ts_create.ts", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    TEST_ASSERT(fd >= 0, "temp file opened");

    ts_writer_t *w = ts_writer_create(fd);
    TEST_ASSERT(w != NULL, "ts_writer created");
    TEST_ASSERT(ts_writer_bytes_written(w) == 0, "initial bytes == 0");

    ts_writer_destroy(w);
    close(fd);
    remove("/tmp/test_ts_create.ts");
    ts_writer_destroy(NULL); /* must not crash */

    TEST_PASS("ts_writer create/destroy");
    return 0;
}

static int test_ts_writer_pat_pmt(void) {
    printf("\n=== test_ts_writer_pat_pmt ===\n");

    int fd = open("/tmp/test_ts_patpmt.ts", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    TEST_ASSERT(fd >= 0, "temp file");

    ts_writer_t *w = ts_writer_create(fd);
    int rc = ts_writer_write_pat_pmt(w);
    TEST_ASSERT(rc == 0, "write_pat_pmt returns 0");

    /* Exactly 2 TS packets = 2 * 188 bytes */
    TEST_ASSERT(ts_writer_bytes_written(w) == 2 * HLS_TS_PACKET_SZ,
                "2 TS packets written for PAT+PMT");

    ts_writer_destroy(w);
    close(fd);

    /* Verify sync bytes in file */
    FILE *f = fopen("/tmp/test_ts_patpmt.ts", "rb");
    TEST_ASSERT(f != NULL, "file readable");
    uint8_t buf[2 * HLS_TS_PACKET_SZ];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    TEST_ASSERT(n == 2 * HLS_TS_PACKET_SZ, "correct file size");
    TEST_ASSERT(buf[0] == HLS_TS_SYNC_BYTE, "PAT packet sync byte");
    TEST_ASSERT(buf[HLS_TS_PACKET_SZ] == HLS_TS_SYNC_BYTE, "PMT packet sync byte");

    remove("/tmp/test_ts_patpmt.ts");
    TEST_PASS("ts_writer PAT+PMT packets written");
    return 0;
}

static int test_ts_writer_pes(void) {
    printf("\n=== test_ts_writer_pes ===\n");

    int fd = open("/tmp/test_ts_pes.ts", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    ts_writer_t *w = ts_writer_create(fd);

    /* Write 200 bytes of fake NAL data */
    uint8_t fake_nal[200];
    memset(fake_nal, 0x00, sizeof(fake_nal));
    int rc = ts_writer_write_pes(w, fake_nal, sizeof(fake_nal), 90000ULL, true);
    TEST_ASSERT(rc == 0, "write_pes returns 0");
    TEST_ASSERT(ts_writer_bytes_written(w) >= HLS_TS_PACKET_SZ,
                "at least 1 TS packet written");

    /* All written bytes must be multiple of 188 */
    TEST_ASSERT(ts_writer_bytes_written(w) % HLS_TS_PACKET_SZ == 0,
                "bytes written is multiple of TS packet size");

    ts_writer_destroy(w);
    close(fd);
    remove("/tmp/test_ts_pes.ts");
    TEST_PASS("ts_writer PES packets correct size");
    return 0;
}

static int test_ts_writer_null_guards(void) {
    printf("\n=== test_ts_writer_null_guards ===\n");

    TEST_ASSERT(ts_writer_write_pat_pmt(NULL) == -1, "pat_pmt NULL returns -1");
    int fd = open("/tmp/test_ts_ng.ts", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    ts_writer_t *w = ts_writer_create(fd);
    uint8_t d[10] = {0};
    TEST_ASSERT(ts_writer_write_pes(NULL, d, 10, 0, false) == -1,
                "pes NULL writer returns -1");
    TEST_ASSERT(ts_writer_write_pes(w, NULL, 10, 0, false) == -1,
                "pes NULL data returns -1");
    TEST_ASSERT(ts_writer_write_pes(w, d, 0, 0, false) == -1,
                "pes 0-length returns -1");
    ts_writer_destroy(w);
    close(fd);
    remove("/tmp/test_ts_ng.ts");

    TEST_PASS("ts_writer NULL guards");
    return 0;
}

/* ── m3u8_writer tests ───────────────────────────────────────────── */

static int test_m3u8_live(void) {
    printf("\n=== test_m3u8_live ===\n");

    hls_segment_t segs[3] = {
        {"seg00000.ts", 6.0, false},
        {"seg00001.ts", 6.0, false},
        {"seg00002.ts", 6.0, false},
    };

    char buf[4096];
    int n = m3u8_write_live(segs, 3, 5, 6, 0, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "live m3u8 returns positive size");
    TEST_ASSERT(strstr(buf, "#EXTM3U") != NULL, "has #EXTM3U");
    TEST_ASSERT(strstr(buf, "#EXT-X-TARGETDURATION:6") != NULL, "target duration 6");
    TEST_ASSERT(strstr(buf, "seg00000.ts") != NULL, "first segment present");
    TEST_ASSERT(strstr(buf, "seg00002.ts") != NULL, "last segment present");
    TEST_ASSERT(strstr(buf, "#EXT-X-ENDLIST") == NULL, "no ENDLIST in live");

    TEST_PASS("m3u8 live playlist");
    return 0;
}

static int test_m3u8_vod(void) {
    printf("\n=== test_m3u8_vod ===\n");

    hls_segment_t segs[2] = {
        {"seg00000.ts", 6.0, false},
        {"seg00001.ts", 4.5, false},
    };

    char buf[4096];
    int n = m3u8_write_vod(segs, 2, 6, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "VOD m3u8 positive");
    TEST_ASSERT(strstr(buf, "#EXT-X-PLAYLIST-TYPE:VOD") != NULL, "VOD type");
    TEST_ASSERT(strstr(buf, "#EXT-X-ENDLIST") != NULL, "has ENDLIST");
    TEST_ASSERT(strstr(buf, "4.500000") != NULL, "second segment duration");

    TEST_PASS("m3u8 VOD playlist");
    return 0;
}

static int test_m3u8_master(void) {
    printf("\n=== test_m3u8_master ===\n");

    const char *uris[2] = {"hi/index.m3u8", "lo/index.m3u8"};
    int bandwidths[2]   = {4000000, 1000000};

    char buf[4096];
    int n = m3u8_write_master(uris, bandwidths, 2, 1920, 1080, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "master m3u8 positive");
    TEST_ASSERT(strstr(buf, "#EXTM3U") != NULL, "has #EXTM3U");
    TEST_ASSERT(strstr(buf, "BANDWIDTH=4000000") != NULL, "HI bandwidth");
    TEST_ASSERT(strstr(buf, "RESOLUTION=1920x1080") != NULL, "resolution");
    TEST_ASSERT(strstr(buf, "hi/index.m3u8") != NULL, "hi variant URI");

    TEST_PASS("m3u8 master playlist");
    return 0;
}

static int test_m3u8_buffer_too_small(void) {
    printf("\n=== test_m3u8_buffer_too_small ===\n");

    hls_segment_t segs[1] = {{"seg00000.ts", 6.0, false}};
    char buf[4];
    int n = m3u8_write_live(segs, 1, 5, 6, 0, buf, sizeof(buf));
    TEST_ASSERT(n == -1, "too-small buffer returns -1");

    TEST_PASS("m3u8 buffer-too-small guard");
    return 0;
}

/* ── hls_segmenter tests ─────────────────────────────────────────── */

static const char *TMP_DIR = "/tmp";

static int test_segmenter_create(void) {
    printf("\n=== test_segmenter_create ===\n");

    hls_segmenter_config_t cfg = {0};
    snprintf(cfg.output_dir,    sizeof(cfg.output_dir),    "%s", TMP_DIR);
    snprintf(cfg.base_name,     sizeof(cfg.base_name),     "hls_test_seg_");
    snprintf(cfg.playlist_name, sizeof(cfg.playlist_name), "hls_test.m3u8");
    cfg.target_duration_s = 6;
    cfg.window_size       = 3;
    cfg.vod_mode          = false;

    hls_segmenter_t *seg = hls_segmenter_create(&cfg);
    TEST_ASSERT(seg != NULL, "segmenter created");
    TEST_ASSERT(hls_segmenter_segment_count(seg) == 0, "initial count 0");
    hls_segmenter_destroy(seg);
    hls_segmenter_destroy(NULL);

    TEST_PASS("hls_segmenter create/destroy");
    return 0;
}

static int test_segmenter_lifecycle(void) {
    printf("\n=== test_segmenter_lifecycle ===\n");

    hls_segmenter_config_t cfg = {0};
    snprintf(cfg.output_dir,    sizeof(cfg.output_dir),    "%s", TMP_DIR);
    snprintf(cfg.base_name,     sizeof(cfg.base_name),     "hls_lc_seg_");
    snprintf(cfg.playlist_name, sizeof(cfg.playlist_name), "hls_lc.m3u8");
    cfg.target_duration_s = 6;
    cfg.window_size       = 5;

    hls_segmenter_t *seg = hls_segmenter_create(&cfg);
    TEST_ASSERT(seg != NULL, "segmenter created");

    /* Open, write some fake data, close */
    int rc = hls_segmenter_open_segment(seg);
    TEST_ASSERT(rc == 0, "open segment returns 0");

    uint8_t nal[512];
    memset(nal, 0, sizeof(nal));
    rc = hls_segmenter_write(seg, nal, sizeof(nal), 0ULL, true);
    TEST_ASSERT(rc == 0, "write frame returns 0");

    rc = hls_segmenter_close_segment(seg, 6.0);
    TEST_ASSERT(rc == 0, "close segment returns 0");
    TEST_ASSERT(hls_segmenter_segment_count(seg) == 1, "1 segment completed");

    /* Update manifest */
    rc = hls_segmenter_update_manifest(seg);
    TEST_ASSERT(rc == 0, "update manifest returns 0");

    /* Verify M3U8 file exists */
    char m3u8_path[HLS_MAX_PATH + HLS_MAX_SEG_NAME + 2];
    snprintf(m3u8_path, sizeof(m3u8_path), "%s/%s", TMP_DIR, "hls_lc.m3u8");
    FILE *f = fopen(m3u8_path, "r");
    TEST_ASSERT(f != NULL, "M3U8 file created");
    char contents[4096] = {0};
    fread(contents, 1, sizeof(contents) - 1, f);
    fclose(f);
    TEST_ASSERT(strstr(contents, "#EXTM3U") != NULL, "M3U8 has header");
    TEST_ASSERT(strstr(contents, "hls_lc_seg_") != NULL, "segment filename in M3U8");

    /* Cleanup */
    remove(m3u8_path);
    char seg_path[HLS_MAX_PATH + HLS_MAX_SEG_NAME + 2];
    snprintf(seg_path, sizeof(seg_path), "%s/hls_lc_seg_0.ts", TMP_DIR);
    remove(seg_path);

    hls_segmenter_destroy(seg);
    TEST_PASS("hls_segmenter open/write/close/manifest lifecycle");
    return 0;
}

static int test_segmenter_vod(void) {
    printf("\n=== test_segmenter_vod ===\n");

    hls_segmenter_config_t cfg = {0};
    snprintf(cfg.output_dir,    sizeof(cfg.output_dir),    "%s", TMP_DIR);
    snprintf(cfg.base_name,     sizeof(cfg.base_name),     "hls_vod_seg_");
    snprintf(cfg.playlist_name, sizeof(cfg.playlist_name), "hls_vod.m3u8");
    cfg.target_duration_s = 6;
    cfg.window_size       = 5;
    cfg.vod_mode          = true;

    hls_segmenter_t *seg = hls_segmenter_create(&cfg);
    hls_segmenter_open_segment(seg);
    uint8_t nal[64] = {0};
    hls_segmenter_write(seg, nal, sizeof(nal), 0ULL, true);
    hls_segmenter_close_segment(seg, 6.0);
    int rc = hls_segmenter_update_manifest(seg);
    TEST_ASSERT(rc == 0, "VOD manifest written");

    char m3u8_path[HLS_MAX_PATH + HLS_MAX_SEG_NAME + 2];
    snprintf(m3u8_path, sizeof(m3u8_path), "%s/hls_vod.m3u8", TMP_DIR);
    FILE *f = fopen(m3u8_path, "r");
    TEST_ASSERT(f != NULL, "VOD M3U8 exists");
    char contents[4096] = {0};
    fread(contents, 1, sizeof(contents) - 1, f);
    fclose(f);
    TEST_ASSERT(strstr(contents, "#EXT-X-ENDLIST") != NULL, "VOD has ENDLIST");

    remove(m3u8_path);
    char seg_path[HLS_MAX_PATH + HLS_MAX_SEG_NAME + 2];
    snprintf(seg_path, sizeof(seg_path), "%s/hls_vod_seg_0.ts", TMP_DIR);
    remove(seg_path);

    hls_segmenter_destroy(seg);
    TEST_PASS("hls_segmenter VOD manifest");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_ts_writer_create();
    failures += test_ts_writer_pat_pmt();
    failures += test_ts_writer_pes();
    failures += test_ts_writer_null_guards();

    failures += test_m3u8_live();
    failures += test_m3u8_vod();
    failures += test_m3u8_master();
    failures += test_m3u8_buffer_too_small();

    failures += test_segmenter_create();
    failures += test_segmenter_lifecycle();
    failures += test_segmenter_vod();

    printf("\n");
    if (failures == 0)
        printf("ALL HLS TESTS PASSED\n");
    else
        printf("%d HLS TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
