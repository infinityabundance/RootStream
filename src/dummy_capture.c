/*
 * dummy_capture.c - Test pattern generator
 * 
 * Generates test images for development/testing:
 * - Allows pipeline validation without real display hardware
 * - Perfect for CI/headless systems
 * - Generates animated patterns for testing
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>

static uint64_t frame_counter = 0;
static char last_error[256] = {0};

const char* rootstream_get_error_dummy(void) {
    return last_error;
}

static void set_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(last_error, sizeof(last_error), fmt, args);
    va_end(args);
}

/*
 * Initialize dummy capture with configurable resolution
 */
int rootstream_capture_init_dummy(rootstream_ctx_t *ctx) {
    if (!ctx) {
        set_error("Invalid context");
        return -1;
    }

    /* Use existing display dimensions if set, otherwise default to 1920x1080 */
    if (ctx->display.width == 0 || ctx->display.height == 0) {
        ctx->display.width = 1920;
        ctx->display.height = 1080;
    }
    
    ctx->display.refresh_rate = 60;
    snprintf(ctx->display.name, sizeof(ctx->display.name), "Dummy-TestPattern");
    ctx->display.fd = -1;

    /* Allocate frame buffer */
    size_t frame_size = ctx->display.width * ctx->display.height * 4; /* RGBA */
    ctx->current_frame.data = malloc(frame_size);
    if (!ctx->current_frame.data) {
        set_error("Cannot allocate frame buffer");
        return -1;
    }

    ctx->current_frame.width = ctx->display.width;
    ctx->current_frame.height = ctx->display.height;
    ctx->current_frame.size = frame_size;
    ctx->current_frame.capacity = frame_size;
    ctx->current_frame.format = 0x34325258; /* DRM_FORMAT_XRGB8888 */

    frame_counter = 0;

    printf("✓ Dummy test pattern initialized: %dx%d @ %d Hz\n",
           ctx->display.width, ctx->display.height, ctx->display.refresh_rate);

    return 0;
}

/*
 * Generate test pattern frame
 * Creates an animated gradient with moving elements
 */
int rootstream_capture_frame_dummy(rootstream_ctx_t *ctx, frame_buffer_t *frame) {
    if (!ctx || !frame) {
        set_error("Invalid arguments");
        return -1;
    }

    uint32_t width = ctx->display.width;
    uint32_t height = ctx->display.height;
    uint8_t *data = frame->data;

    /* Animate based on frame counter */
    double time = frame_counter / 60.0;
    int offset_x = (int)(sin(time) * 100.0);
    int offset_y = (int)(cos(time * 0.7) * 100.0);

    /* Generate test pattern */
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t idx = (y * width + x) * 4;

            /* Create gradient with moving pattern */
            int px = (int)x + offset_x;
            int py = (int)y + offset_y;

            /* Color bars pattern */
            if (y < height / 4) {
                /* Top quarter: horizontal color bars */
                int bar = (x * 8) / width;
                switch (bar) {
                    case 0: data[idx] = 255; data[idx+1] = 255; data[idx+2] = 255; break; /* White */
                    case 1: data[idx] = 255; data[idx+1] = 255; data[idx+2] = 0;   break; /* Yellow */
                    case 2: data[idx] = 0;   data[idx+1] = 255; data[idx+2] = 255; break; /* Cyan */
                    case 3: data[idx] = 0;   data[idx+1] = 255; data[idx+2] = 0;   break; /* Green */
                    case 4: data[idx] = 255; data[idx+1] = 0;   data[idx+2] = 255; break; /* Magenta */
                    case 5: data[idx] = 255; data[idx+1] = 0;   data[idx+2] = 0;   break; /* Red */
                    case 6: data[idx] = 0;   data[idx+1] = 0;   data[idx+2] = 255; break; /* Blue */
                    case 7: data[idx] = 0;   data[idx+1] = 0;   data[idx+2] = 0;   break; /* Black */
                    default: data[idx] = 128; data[idx+1] = 128; data[idx+2] = 128; break;
                }
            } else if (y < height / 2) {
                /* Second quarter: animated gradient */
                uint8_t r = (uint8_t)((px % 256 + frame_counter) % 256);
                uint8_t g = (uint8_t)((py % 256 + frame_counter / 2) % 256);
                uint8_t b = (uint8_t)(((px + py) % 256 + frame_counter / 3) % 256);
                data[idx] = r;
                data[idx+1] = g;
                data[idx+2] = b;
            } else if (y < (3 * height) / 4) {
                /* Third quarter: checkerboard */
                int check_size = 32;
                int cx = (px / check_size) % 2;
                int cy = (py / check_size) % 2;
                uint8_t color = (cx ^ cy) ? 255 : 64;
                data[idx] = color;
                data[idx+1] = color;
                data[idx+2] = color;
            } else {
                /* Bottom quarter: solid color with frame counter */
                uint8_t intensity = (uint8_t)((frame_counter % 256));
                data[idx] = intensity;
                data[idx+1] = 128;
                data[idx+2] = 255 - intensity;
            }

            data[idx+3] = 255; /* Alpha */
        }
    }

    /* Set frame metadata */
    frame->width = width;
    frame->height = height;
    frame->pitch = width * 4;
    frame->format = ctx->current_frame.format;

    /* Get timestamp */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    frame->timestamp = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;

    frame_counter++;
    ctx->frames_captured++;
    return 0;
}

/*
 * Cleanup dummy capture
 */
void rootstream_capture_cleanup_dummy(rootstream_ctx_t *ctx) {
    if (!ctx)
        return;

    if (ctx->current_frame.data) {
        free(ctx->current_frame.data);
        ctx->current_frame.data = NULL;
    }

    frame_counter = 0;
}
