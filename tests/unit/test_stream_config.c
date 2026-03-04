/*
 * test_stream_config.c — Unit tests for PHASE-54 Stream Config Serialiser
 *
 * Tests stream_config (encode/decode/equals/default), config_serialiser
 * (versioned envelope encode/decode/bad-magic/version-mismatch), and
 * config_export (JSON rendering, codec/proto name helpers).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../src/stream_config/stream_config.h"
#include "../../src/stream_config/config_serialiser.h"
#include "../../src/stream_config/config_export.h"

/* ── Test macros ─────────────────────────────────────────────────── */

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) printf("PASS: %s\n", (msg))

/* ── stream_config tests ─────────────────────────────────────────── */

static int test_config_roundtrip(void) {
    printf("\n=== test_config_roundtrip ===\n");

    stream_config_t orig;
    stream_config_default(&orig);
    orig.flags = SCFG_FLAG_ENCRYPTED | SCFG_FLAG_HW_ENCODE;
    orig.transport_proto = SCFG_PROTO_TCP;
    orig.video_bitrate_kbps = 8000;

    uint8_t buf[SCFG_HDR_SIZE];
    int n = stream_config_encode(&orig, buf, sizeof(buf));
    TEST_ASSERT(n == SCFG_HDR_SIZE, "encode returns SCFG_HDR_SIZE");

    stream_config_t decoded;
    int rc = stream_config_decode(buf, (size_t)n, &decoded);
    TEST_ASSERT(rc == 0, "decode ok");
    TEST_ASSERT(stream_config_equals(&orig, &decoded), "configs equal after round-trip");
    TEST_ASSERT(decoded.video_bitrate_kbps == 8000, "bitrate preserved");
    TEST_ASSERT(decoded.flags == (SCFG_FLAG_ENCRYPTED | SCFG_FLAG_HW_ENCODE),
                "flags preserved");
    TEST_ASSERT(decoded.transport_proto == SCFG_PROTO_TCP, "proto preserved");

    TEST_PASS("stream_config encode/decode round-trip");
    return 0;
}

static int test_config_bad_magic(void) {
    printf("\n=== test_config_bad_magic ===\n");

    uint8_t buf[SCFG_HDR_SIZE] = {0};
    stream_config_t cfg;
    TEST_ASSERT(stream_config_decode(buf, sizeof(buf), &cfg) == -1,
                "bad magic → -1");

    TEST_PASS("stream_config bad magic rejected");
    return 0;
}

static int test_config_equals(void) {
    printf("\n=== test_config_equals ===\n");

    stream_config_t a, b;
    stream_config_default(&a);
    stream_config_default(&b);
    TEST_ASSERT(stream_config_equals(&a, &b), "default configs equal");

    b.video_fps = 60;
    TEST_ASSERT(!stream_config_equals(&a, &b), "different fps → not equal");
    TEST_ASSERT(!stream_config_equals(NULL, &b), "NULL a → false");
    TEST_ASSERT(!stream_config_equals(&a, NULL), "NULL b → false");

    TEST_PASS("stream_config equals");
    return 0;
}

static int test_config_default(void) {
    printf("\n=== test_config_default ===\n");

    stream_config_t cfg;
    stream_config_default(&cfg);
    TEST_ASSERT(cfg.video_codec    == SCFG_VCODEC_H264, "default h264");
    TEST_ASSERT(cfg.audio_codec    == SCFG_ACODEC_OPUS, "default opus");
    TEST_ASSERT(cfg.video_width    == 1280, "default width");
    TEST_ASSERT(cfg.video_height   == 720,  "default height");
    TEST_ASSERT(cfg.video_fps      == 30,   "default fps");
    TEST_ASSERT(cfg.transport_port == 5900, "default port");

    TEST_PASS("stream_config default values");
    return 0;
}

/* ── config_serialiser tests ─────────────────────────────────────── */

static int test_serialiser_roundtrip(void) {
    printf("\n=== test_serialiser_roundtrip ===\n");

    stream_config_t orig;
    stream_config_default(&orig);
    orig.video_codec = SCFG_VCODEC_H265;

    uint8_t buf[128];
    int n = config_serialiser_encode(&orig, buf, sizeof(buf));
    TEST_ASSERT(n == config_serialiser_total_size(), "encoded size matches total_size");

    stream_config_t decoded;
    int rc = config_serialiser_decode(buf, (size_t)n, &decoded);
    TEST_ASSERT(rc == CSER_OK, "decode ok");
    TEST_ASSERT(stream_config_equals(&orig, &decoded), "configs equal");

    TEST_PASS("config_serialiser encode/decode round-trip");
    return 0;
}

static int test_serialiser_bad_magic(void) {
    printf("\n=== test_serialiser_bad_magic ===\n");

    uint8_t buf[128] = {0};
    stream_config_t cfg;
    TEST_ASSERT(config_serialiser_decode(buf, sizeof(buf), &cfg) == CSER_ERR_BAD_MAGIC,
                "bad magic → CSER_ERR_BAD_MAGIC");

    TEST_PASS("config_serialiser bad magic rejected");
    return 0;
}

static int test_serialiser_version_mismatch(void) {
    printf("\n=== test_serialiser_version_mismatch ===\n");

    stream_config_t orig; stream_config_default(&orig);
    uint8_t buf[128];
    config_serialiser_encode(&orig, buf, sizeof(buf));

    /* Corrupt major version */
    buf[5] = 0xFF; /* version major byte (big-endian in the 16-bit field: byte 5 = major) */
    stream_config_t cfg;
    int rc = config_serialiser_decode(buf, sizeof(buf), &cfg);
    TEST_ASSERT(rc == CSER_ERR_VERSION, "major version mismatch → CSER_ERR_VERSION");

    TEST_PASS("config_serialiser version mismatch");
    return 0;
}

static int test_serialiser_null_guards(void) {
    printf("\n=== test_serialiser_null_guards ===\n");

    uint8_t buf[128];
    stream_config_t cfg; stream_config_default(&cfg);
    TEST_ASSERT(config_serialiser_encode(NULL, buf, sizeof(buf)) == CSER_ERR_NULL,
                "encode NULL cfg");
    TEST_ASSERT(config_serialiser_encode(&cfg, NULL, 0) == CSER_ERR_NULL,
                "encode NULL buf");
    TEST_ASSERT(config_serialiser_decode(NULL, 0, &cfg) == CSER_ERR_NULL,
                "decode NULL buf");

    TEST_PASS("config_serialiser NULL guards");
    return 0;
}

/* ── config_export tests ─────────────────────────────────────────── */

static int test_export_json(void) {
    printf("\n=== test_export_json ===\n");

    stream_config_t cfg; stream_config_default(&cfg);
    cfg.flags = SCFG_FLAG_ENCRYPTED;

    char buf[1024];
    int n = config_export_json(&cfg, buf, sizeof(buf));
    TEST_ASSERT(n > 0, "export JSON positive");
    TEST_ASSERT(buf[0] == '{', "starts with {");
    TEST_ASSERT(buf[n-1] == '}', "ends with }");
    TEST_ASSERT(strstr(buf, "\"video_codec\":\"h264\"") != NULL, "h264 in JSON");
    TEST_ASSERT(strstr(buf, "\"audio_codec\":\"opus\"") != NULL, "opus in JSON");
    TEST_ASSERT(strstr(buf, "\"transport_proto\":\"udp\"") != NULL, "udp in JSON");
    TEST_ASSERT(strstr(buf, "\"encrypted\":true") != NULL, "encrypted flag in JSON");

    /* Buffer too small */
    n = config_export_json(&cfg, buf, 5);
    TEST_ASSERT(n == -1, "too-small buffer → -1");

    TEST_PASS("config_export JSON");
    return 0;
}

static int test_export_codec_names(void) {
    printf("\n=== test_export_codec_names ===\n");

    TEST_ASSERT(strcmp(config_vcodec_name(SCFG_VCODEC_RAW),  "raw")  == 0, "raw");
    TEST_ASSERT(strcmp(config_vcodec_name(SCFG_VCODEC_H264), "h264") == 0, "h264");
    TEST_ASSERT(strcmp(config_vcodec_name(SCFG_VCODEC_H265), "h265") == 0, "h265");
    TEST_ASSERT(strcmp(config_vcodec_name(SCFG_VCODEC_AV1),  "av1")  == 0, "av1");
    TEST_ASSERT(strcmp(config_vcodec_name(SCFG_VCODEC_VP9),  "vp9")  == 0, "vp9");
    TEST_ASSERT(strcmp(config_vcodec_name(99), "unknown") == 0, "unknown vcodec");

    TEST_ASSERT(strcmp(config_acodec_name(SCFG_ACODEC_OPUS), "opus") == 0, "opus");
    TEST_ASSERT(strcmp(config_acodec_name(99), "unknown") == 0, "unknown acodec");

    TEST_ASSERT(strcmp(config_proto_name(SCFG_PROTO_UDP),  "udp")  == 0, "udp");
    TEST_ASSERT(strcmp(config_proto_name(SCFG_PROTO_TCP),  "tcp")  == 0, "tcp");
    TEST_ASSERT(strcmp(config_proto_name(SCFG_PROTO_QUIC), "quic") == 0, "quic");
    TEST_ASSERT(strcmp(config_proto_name(99), "unknown") == 0, "unknown proto");

    TEST_PASS("config_export codec/proto names");
    return 0;
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    int failures = 0;

    failures += test_config_roundtrip();
    failures += test_config_bad_magic();
    failures += test_config_equals();
    failures += test_config_default();

    failures += test_serialiser_roundtrip();
    failures += test_serialiser_bad_magic();
    failures += test_serialiser_version_mismatch();
    failures += test_serialiser_null_guards();

    failures += test_export_json();
    failures += test_export_codec_names();

    printf("\n");
    if (failures == 0)
        printf("ALL STREAM CONFIG TESTS PASSED\n");
    else
        printf("%d STREAM CONFIG TEST(S) FAILED\n", failures);
    return failures ? 1 : 0;
}
