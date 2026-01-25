/*
 * vaapi_stub.c - Stubbed VA-API encoder for environments without libva
 *
 * This lets HEADLESS/CI builds compile without VA-API headers or libraries.
 */

#include "../include/rootstream.h"
#include <stdio.h>

int rootstream_encoder_init(rootstream_ctx_t *ctx, encoder_type_t type) {
    (void)ctx;
    (void)type;
    fprintf(stderr, "ERROR: VA-API encoder unavailable (libva not found at build time)\n");
    fprintf(stderr, "FIX: Install libva/libva-drm development packages and rebuild\n");
    return -1;
}

int rootstream_encode_frame(rootstream_ctx_t *ctx, frame_buffer_t *in,
                           uint8_t *out, size_t *out_size) {
    (void)ctx;
    (void)in;
    (void)out;
    (void)out_size;
    fprintf(stderr, "ERROR: Cannot encode frame without VA-API support\n");
    return -1;
}

void rootstream_encoder_cleanup(rootstream_ctx_t *ctx) {
    (void)ctx;
}
