/*
 * x11_capture.c - X11 SHM screen grab fallback
 * 
 * Fallback capture backend when DRM is unavailable:
 * - Works on X11 systems without DRM access
 * - Uses XGetImage or XShm for screen capture
 * - Slower than DRM but more compatible
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#ifdef HAVE_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct {
    Display *display;
    Window root;
    int screen;
} x11_capture_ctx_t;

static x11_capture_ctx_t x11_ctx = {0};
static char last_error[256] = {0};

const char* rootstream_get_error_x11(void) {
    return last_error;
}

static void set_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(last_error, sizeof(last_error), fmt, args);
    va_end(args);
}

/*
 * Initialize X11 capture
 */
int rootstream_capture_init_x11(rootstream_ctx_t *ctx) {
    if (!ctx) {
        set_error("Invalid context");
        return -1;
    }

    /* Open X display */
    x11_ctx.display = XOpenDisplay(NULL);
    if (!x11_ctx.display) {
        set_error("Cannot open X display (DISPLAY not set or X11 not available)");
        return -1;
    }

    x11_ctx.screen = DefaultScreen(x11_ctx.display);
    x11_ctx.root = RootWindow(x11_ctx.display, x11_ctx.screen);

    /* Get screen dimensions */
    XWindowAttributes attrs;
    if (XGetWindowAttributes(x11_ctx.display, x11_ctx.root, &attrs) == 0) {
        set_error("Cannot get root window attributes");
        XCloseDisplay(x11_ctx.display);
        x11_ctx.display = NULL;
        return -1;
    }

    /* Initialize display info */
    ctx->display.width = attrs.width;
    ctx->display.height = attrs.height;
    ctx->display.refresh_rate = 60;  /* Assume 60Hz */
    snprintf(ctx->display.name, sizeof(ctx->display.name), "X11-Screen-%d", x11_ctx.screen);
    ctx->display.fd = -1;  /* No file descriptor for X11 */

    /* Allocate frame buffer */
    size_t frame_size = ctx->display.width * ctx->display.height * 4; /* RGBA */
    ctx->current_frame.data = malloc(frame_size);
    if (!ctx->current_frame.data) {
        set_error("Cannot allocate frame buffer");
        XCloseDisplay(x11_ctx.display);
        x11_ctx.display = NULL;
        return -1;
    }

    ctx->current_frame.width = ctx->display.width;
    ctx->current_frame.height = ctx->display.height;
    ctx->current_frame.size = frame_size;
    ctx->current_frame.capacity = frame_size;
    ctx->current_frame.format = 0x34325258; /* DRM_FORMAT_XRGB8888 */

    printf("✓ X11 SHM capture initialized: %dx%d\n",
           ctx->display.width, ctx->display.height);

    return 0;
}

/*
 * Capture frame using X11 XGetImage
 */
int rootstream_capture_frame_x11(rootstream_ctx_t *ctx, frame_buffer_t *frame) {
    if (!ctx || !frame || !x11_ctx.display) {
        set_error("Invalid arguments or X11 not initialized");
        return -1;
    }

    /* Capture screen using XGetImage */
    XImage *image = XGetImage(x11_ctx.display, x11_ctx.root,
                              0, 0,
                              ctx->display.width, ctx->display.height,
                              AllPlanes, ZPixmap);
    if (!image) {
        set_error("XGetImage failed");
        return -1;
    }

    /* Convert to RGBA format */
    uint8_t *dst = frame->data;
    
    for (unsigned int y = 0; y < ctx->display.height; y++) {
        for (unsigned int x = 0; x < ctx->display.width; x++) {
            unsigned long pixel = XGetPixel(image, x, y);
            
            /* Extract RGB components (assuming 24-bit or 32-bit color) */
            dst[0] = (pixel >> 16) & 0xFF;  /* R */
            dst[1] = (pixel >> 8) & 0xFF;   /* G */
            dst[2] = pixel & 0xFF;          /* B */
            dst[3] = 0xFF;                  /* A */
            
            dst += 4;
        }
    }

    XDestroyImage(image);

    /* Set frame metadata */
    frame->width = ctx->display.width;
    frame->height = ctx->display.height;
    frame->pitch = ctx->display.width * 4;
    frame->format = ctx->current_frame.format;

    /* Get timestamp */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    frame->timestamp = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;

    ctx->frames_captured++;
    return 0;
}

/*
 * Cleanup X11 capture
 */
void rootstream_capture_cleanup_x11(rootstream_ctx_t *ctx) {
    if (!ctx)
        return;

    if (ctx->current_frame.data) {
        free(ctx->current_frame.data);
        ctx->current_frame.data = NULL;
    }

    if (x11_ctx.display) {
        XCloseDisplay(x11_ctx.display);
        x11_ctx.display = NULL;
    }
}

#else /* !HAVE_X11 */

/* Stub implementation when X11 is not available */

static char last_error[256] __attribute__((unused)) = "X11 support not compiled in";

int rootstream_capture_init_x11(rootstream_ctx_t *ctx) {
    (void)ctx;
    return -1;
}

int rootstream_capture_frame_x11(rootstream_ctx_t *ctx, frame_buffer_t *frame) {
    (void)ctx;
    (void)frame;
    return -1;
}

void rootstream_capture_cleanup_x11(rootstream_ctx_t *ctx) {
    (void)ctx;
}

#endif /* HAVE_X11 */
