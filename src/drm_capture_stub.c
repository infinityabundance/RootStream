/*
 * drm_capture_stub.c - Stubbed DRM capture for NO_DRM builds
 *
 * Allows compiling without libdrm headers/libraries.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <string.h>

static char last_error[256];

const char* rootstream_get_error(void) {
    return last_error;
}

static void set_error(const char *msg) {
    strncpy(last_error, msg, sizeof(last_error) - 1);
    last_error[sizeof(last_error) - 1] = '\0';
}

int rootstream_detect_displays(display_info_t *displays, int max_displays) {
    (void)displays;
    (void)max_displays;
    set_error("DRM capture unavailable (NO_DRM build)");
    return -1;
}

int rootstream_select_display(rootstream_ctx_t *ctx, int display_index) {
    (void)ctx;
    (void)display_index;
    set_error("Display selection unavailable (NO_DRM build)");
    return -1;
}

int rootstream_capture_init(rootstream_ctx_t *ctx) {
    (void)ctx;
    set_error("Capture init unavailable (NO_DRM build)");
    return -1;
}

int rootstream_capture_frame(rootstream_ctx_t *ctx, frame_buffer_t *frame) {
    (void)ctx;
    (void)frame;
    set_error("Capture frame unavailable (NO_DRM build)");
    return -1;
}

void rootstream_capture_cleanup(rootstream_ctx_t *ctx) {
    (void)ctx;
}
