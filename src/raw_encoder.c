/*
 * raw_encoder.c - Raw frame pass-through encoder for debugging
 *
 * Passes raw RGBA frames with minimal overhead. Huge bandwidth, but:
 * - Validates full pipeline without compression
 * - Useful for debugging encoder issues
 * - Never fails (always available)
 *
 * Frame format:
 *   [Header: 24 bytes]
 *   [Raw RGBA data]
 *
 * Header structure:
 *   uint32_t magic         - 0x52535452 "RSTR"
 *   uint32_t width         - Frame width
 *   uint32_t height        - Frame height
 *   uint32_t format        - Pixel format (1 = RGBA)
 *   uint64_t timestamp_us  - Capture timestamp
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAW_MAGIC 0x52535452  /* "RSTR" */
#define RAW_FORMAT_RGBA 1

typedef struct {
    uint32_t magic;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint64_t timestamp_us;
} raw_header_t;

typedef struct {
    int width;
    int height;
    uint64_t frame_count;
} raw_ctx_t;

/*
 * Initialize raw encoder (always succeeds)
 */
int rootstream_encoder_init_raw(rootstream_ctx_t *ctx, codec_type_t codec) {
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context\n");
        return -1;
    }

    (void)codec;  /* Raw encoder ignores codec type */

    /* Allocate raw encoder context */
    raw_ctx_t *raw = calloc(1, sizeof(raw_ctx_t));
    if (!raw) {
        fprintf(stderr, "ERROR: Cannot allocate raw encoder context\n");
        return -1;
    }

    raw->width = ctx->display.width;
    raw->height = ctx->display.height;
    raw->frame_count = 0;

    ctx->encoder.type = ENCODER_RAW;
    ctx->encoder.codec = codec;  /* Store but not used */
    ctx->encoder.hw_ctx = raw;
    ctx->encoder.low_latency = true;
    ctx->encoder.max_output_size = sizeof(raw_header_t) + 
                                   (size_t)raw->width * raw->height * 4;

    size_t bandwidth_mb_per_sec = (ctx->encoder.max_output_size * ctx->display.refresh_rate) / (1024 * 1024);

    printf("✓ Raw pass-through encoder ready: %dx%d (debug mode)\n",
           raw->width, raw->height);
    printf("  ⚠ WARNING: Uncompressed - ~%zu MB/s bandwidth required\n", bandwidth_mb_per_sec);
    printf("  Use only for testing/debugging on high-bandwidth networks\n");

    return 0;
}

/*
 * Encode raw frame (just copy with header)
 */
int rootstream_encode_frame_raw(rootstream_ctx_t *ctx, frame_buffer_t *in,
                                uint8_t *out, size_t *out_size) {
    if (!ctx || !in || !out || !out_size) {
        return -1;
    }

    raw_ctx_t *raw = (raw_ctx_t*)ctx->encoder.hw_ctx;
    if (!raw) {
        fprintf(stderr, "ERROR: Raw encoder not initialized\n");
        return -1;
    }

    /* Build header */
    raw_header_t header = {
        .magic = RAW_MAGIC,
        .width = in->width,
        .height = in->height,
        .format = RAW_FORMAT_RGBA,
        .timestamp_us = in->timestamp
    };

    /* Copy header */
    memcpy(out, &header, sizeof(header));

    /* Copy raw frame data */
    size_t data_size = (size_t)in->width * in->height * 4;
    memcpy(out + sizeof(header), in->data, data_size);

    *out_size = sizeof(header) + data_size;

    /* All frames are "keyframes" in raw mode */
    in->is_keyframe = true;
    
    raw->frame_count++;
    
    return 0;
}

/*
 * Cleanup raw encoder
 */
void rootstream_encoder_cleanup_raw(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->encoder.hw_ctx) {
        return;
    }

    raw_ctx_t *raw = (raw_ctx_t*)ctx->encoder.hw_ctx;
    free(raw);
    ctx->encoder.hw_ctx = NULL;
}
