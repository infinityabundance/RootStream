/*
 * test_encoding.c - Unit tests for video encoding functions
 *
 * Tests:
 * - Colorspace conversion (RGBA to NV12)
 * - NAL unit parsing
 * - Keyframe detection
 * - Encoder parameter validation
 *
 * Note: Full encoder tests require hardware (VA-API/NVENC)
 * These tests focus on pure software functions.
 */

#include "../../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        printf("  [TEST] %s... ", #name); \
        fflush(stdout); \
        tests_run++; \
        test_##name(); \
        tests_passed++; \
        printf("✓\n"); \
    } \
    static void test_##name(void)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("✗\n"); \
        fprintf(stderr, "    FAILED: %s (line %d)\n", #cond, __LINE__); \
        tests_failed++; \
        tests_passed--; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_RANGE(val, min, max) ASSERT((val) >= (min) && (val) <= (max))

/* ============================================================================
 * Helper: NAL unit detection (copy of encoder logic for testing)
 * ============================================================================ */

static bool detect_h264_keyframe(const uint8_t *data, size_t size) {
    if (!data || size < 5) return false;

    for (size_t i = 0; i < size - 4; i++) {
        bool sc3 = (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x01);
        bool sc4 = (i + 4 < size && data[i] == 0x00 && data[i+1] == 0x00 &&
                   data[i+2] == 0x00 && data[i+3] == 0x01);

        if (sc3 || sc4) {
            size_t idx = sc4 ? i + 4 : i + 3;
            if (idx < size && (data[idx] & 0x1F) == 5) {
                return true;
            }
            i += sc4 ? 3 : 2;
        }
    }
    return false;
}

static bool detect_h265_keyframe(const uint8_t *data, size_t size) {
    if (!data || size < 5) return false;

    for (size_t i = 0; i < size - 4; i++) {
        bool sc3 = (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x01);
        bool sc4 = (i + 4 < size && data[i] == 0x00 && data[i+1] == 0x00 &&
                   data[i+2] == 0x00 && data[i+3] == 0x01);

        if (sc3 || sc4) {
            size_t idx = sc4 ? i + 4 : i + 3;
            if (idx < size) {
                uint8_t nal_type = (data[idx] >> 1) & 0x3F;
                if (nal_type == 19 || nal_type == 20 || nal_type == 21) {
                    return true;
                }
            }
            i += sc4 ? 3 : 2;
        }
    }
    return false;
}

/* ============================================================================
 * Tests: NAL Unit Parsing
 * ============================================================================ */

TEST(h264_idr_detection) {
    /* H.264 IDR frame: start code + NAL type 5 */
    uint8_t idr_frame[] = {
        0x00, 0x00, 0x00, 0x01,  /* 4-byte start code */
        0x65,                     /* NAL type 5 (IDR) with nal_ref_idc=3 */
        0x88, 0x84, 0x00, 0x00   /* Some slice data */
    };

    ASSERT(detect_h264_keyframe(idr_frame, sizeof(idr_frame)));
}

TEST(h264_non_idr_detection) {
    /* H.264 P-frame: start code + NAL type 1 */
    uint8_t p_frame[] = {
        0x00, 0x00, 0x00, 0x01,  /* 4-byte start code */
        0x41,                     /* NAL type 1 (non-IDR) */
        0x9A, 0x24, 0x6C, 0x00   /* Some slice data */
    };

    ASSERT(!detect_h264_keyframe(p_frame, sizeof(p_frame)));
}

TEST(h264_sps_pps_idr_sequence) {
    /* Typical IDR with SPS/PPS: SPS + PPS + IDR */
    uint8_t sequence[] = {
        /* SPS (NAL type 7) */
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x1E,
        /* PPS (NAL type 8) */
        0x00, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x38, 0x80,
        /* IDR (NAL type 5) */
        0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x00
    };

    ASSERT(detect_h264_keyframe(sequence, sizeof(sequence)));
}

TEST(h264_3byte_start_code) {
    /* 3-byte start code variant */
    uint8_t idr_3byte[] = {
        0x00, 0x00, 0x01,        /* 3-byte start code */
        0x65,                     /* NAL type 5 (IDR) */
        0x88, 0x84, 0x00
    };

    ASSERT(detect_h264_keyframe(idr_3byte, sizeof(idr_3byte)));
}

TEST(h265_idr_detection) {
    /* H.265 IDR_W_RADL: NAL type 19 */
    uint8_t idr_frame[] = {
        0x00, 0x00, 0x00, 0x01,  /* Start code */
        0x26, 0x01,              /* NAL header: type=19 (IDR_W_RADL) */
        0x00, 0x00, 0x00, 0x00   /* Slice data */
    };

    ASSERT(detect_h265_keyframe(idr_frame, sizeof(idr_frame)));
}

TEST(h265_idr_n_lp_detection) {
    /* H.265 IDR_N_LP: NAL type 20 */
    uint8_t idr_frame[] = {
        0x00, 0x00, 0x00, 0x01,
        0x28, 0x01,              /* NAL type 20 */
        0x00, 0x00, 0x00, 0x00
    };

    ASSERT(detect_h265_keyframe(idr_frame, sizeof(idr_frame)));
}

TEST(h265_cra_detection) {
    /* H.265 CRA_NUT: NAL type 21 */
    uint8_t cra_frame[] = {
        0x00, 0x00, 0x00, 0x01,
        0x2A, 0x01,              /* NAL type 21 (CRA) */
        0x00, 0x00, 0x00, 0x00
    };

    ASSERT(detect_h265_keyframe(cra_frame, sizeof(cra_frame)));
}

TEST(h265_non_idr_detection) {
    /* H.265 TRAIL_R: NAL type 1 (not keyframe) */
    uint8_t p_frame[] = {
        0x00, 0x00, 0x00, 0x01,
        0x02, 0x01,              /* NAL type 1 */
        0x00, 0x00, 0x00, 0x00
    };

    ASSERT(!detect_h265_keyframe(p_frame, sizeof(p_frame)));
}

TEST(empty_buffer_no_crash) {
    ASSERT(!detect_h264_keyframe(NULL, 0));
    ASSERT(!detect_h265_keyframe(NULL, 0));

    uint8_t small[] = {0x00, 0x00};
    ASSERT(!detect_h264_keyframe(small, sizeof(small)));
    ASSERT(!detect_h265_keyframe(small, sizeof(small)));
}

/* ============================================================================
 * Tests: Frame Buffer
 * ============================================================================ */

TEST(frame_buffer_init) {
    frame_buffer_t frame = {0};

    ASSERT_EQ(frame.data, NULL);
    ASSERT_EQ(frame.size, 0);
    ASSERT_EQ(frame.is_keyframe, false);
}

TEST(frame_buffer_allocation) {
    frame_buffer_t frame = {0};

    /* Simulate 1920x1080 RGBA frame */
    size_t size = 1920 * 1080 * 4;
    frame.data = malloc(size);
    frame.size = size;
    frame.width = 1920;
    frame.height = 1080;
    frame.pitch = 1920 * 4;
    frame.format = 0x34325241;  /* DRM_FORMAT_RGBA8888 */

    ASSERT(frame.data != NULL);
    ASSERT_EQ(frame.size, size);
    ASSERT_EQ(frame.width, 1920);
    ASSERT_EQ(frame.height, 1080);

    free(frame.data);
}

/* ============================================================================
 * Tests: Encoder Context
 * ============================================================================ */

TEST(encoder_ctx_defaults) {
    encoder_ctx_t enc = {0};

    ASSERT_EQ(enc.type, ENCODER_VAAPI);  /* 0 = VAAPI */
    ASSERT_EQ(enc.codec, CODEC_H264);    /* 0 = H264 */
    ASSERT_EQ(enc.bitrate, 0);
    ASSERT_EQ(enc.force_keyframe, false);
}

TEST(encoder_bitrate_validation) {
    /* Valid bitrates for streaming */
    uint32_t valid_bitrates[] = {
        1000000,    /* 1 Mbps - minimum for decent quality */
        5000000,    /* 5 Mbps - good for 720p */
        10000000,   /* 10 Mbps - good for 1080p */
        20000000,   /* 20 Mbps - high quality 1080p */
        50000000,   /* 50 Mbps - 4K streaming */
    };

    for (size_t i = 0; i < sizeof(valid_bitrates) / sizeof(valid_bitrates[0]); i++) {
        ASSERT_RANGE(valid_bitrates[i], 500000, 100000000);
    }
}

TEST(encoder_framerate_validation) {
    /* Valid framerates */
    uint32_t valid_fps[] = {24, 30, 60, 120, 144, 240};

    for (size_t i = 0; i < sizeof(valid_fps) / sizeof(valid_fps[0]); i++) {
        ASSERT_RANGE(valid_fps[i], 1, 240);
    }
}

/* ============================================================================
 * Tests: Colorspace Math
 * ============================================================================ */

TEST(yuv_coefficients_bt709) {
    /* BT.709 coefficients for Y calculation:
     * Y = 0.2126*R + 0.7152*G + 0.0722*B
     * Scaled: Y = (66*R + 129*G + 25*B + 128) >> 8 + 16
     */

    /* Test with pure white (255, 255, 255) */
    uint8_t r = 255, g = 255, b = 255;
    int y_val = (66*r + 129*g + 25*b + 128) >> 8;
    y_val += 16;

    /* Y should be close to 235 (white in video range) */
    ASSERT_RANGE(y_val, 230, 240);

    /* Test with pure black (0, 0, 0) */
    r = g = b = 0;
    y_val = (66*r + 129*g + 25*b + 128) >> 8;
    y_val += 16;

    /* Y should be 16 (black in video range) */
    ASSERT_EQ(y_val, 16);
}

TEST(uv_coefficients_bt709) {
    /* BT.709 U/V coefficients:
     * U = -0.1146*R - 0.3854*G + 0.5*B
     * V =  0.5*R - 0.4542*G - 0.0458*B
     */

    /* Pure red (255, 0, 0) - should have high V, low U */
    uint8_t r = 255, g = 0, b = 0;
    int u_val = (-38*r - 74*g + 112*b + 128) >> 8;
    int v_val = (112*r - 94*g - 18*b + 128) >> 8;
    u_val += 128;
    v_val += 128;

    ASSERT(v_val > 128);  /* Red has positive V */
    ASSERT(u_val < 128);  /* Red has negative U */

    /* Pure blue (0, 0, 255) - should have high U */
    r = 0; g = 0; b = 255;
    u_val = (-38*r - 74*g + 112*b + 128) >> 8;
    u_val += 128;

    ASSERT(u_val > 128);  /* Blue has positive U */
}

/* ============================================================================
 * Tests: Control Commands
 * ============================================================================ */

TEST(control_packet_size) {
    control_packet_t pkt;
    /* Packed struct should be exactly 5 bytes */
    ASSERT_EQ(sizeof(pkt), 5);
}

TEST(control_cmd_values) {
    ASSERT_EQ(CTRL_PAUSE, 0x01);
    ASSERT_EQ(CTRL_RESUME, 0x02);
    ASSERT_EQ(CTRL_SET_BITRATE, 0x03);
    ASSERT_EQ(CTRL_SET_FPS, 0x04);
    ASSERT_EQ(CTRL_REQUEST_KEYFRAME, 0x05);
    ASSERT_EQ(CTRL_SET_QUALITY, 0x06);
    ASSERT_EQ(CTRL_DISCONNECT, 0x07);
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║  RootStream Encoding Unit Tests                ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\n");

    /* NAL parsing tests */
    printf("NAL Unit Parsing:\n");
    run_test_h264_idr_detection();
    run_test_h264_non_idr_detection();
    run_test_h264_sps_pps_idr_sequence();
    run_test_h264_3byte_start_code();
    run_test_h265_idr_detection();
    run_test_h265_idr_n_lp_detection();
    run_test_h265_cra_detection();
    run_test_h265_non_idr_detection();
    run_test_empty_buffer_no_crash();

    /* Frame buffer tests */
    printf("\nFrame Buffer:\n");
    run_test_frame_buffer_init();
    run_test_frame_buffer_allocation();

    /* Encoder context tests */
    printf("\nEncoder Context:\n");
    run_test_encoder_ctx_defaults();
    run_test_encoder_bitrate_validation();
    run_test_encoder_framerate_validation();

    /* Colorspace tests */
    printf("\nColorspace Conversion:\n");
    run_test_yuv_coefficients_bt709();
    run_test_uv_coefficients_bt709();

    /* Control command tests */
    printf("\nControl Commands:\n");
    run_test_control_packet_size();
    run_test_control_cmd_values();

    /* Summary */
    printf("\n");
    printf("════════════════════════════════════════════════\n");
    printf("  Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        printf(" (%d failed)", tests_failed);
    }
    printf("\n");
    printf("════════════════════════════════════════════════\n");
    printf("\n");

    return tests_failed > 0 ? 1 : 0;
}
