/*
 * drm_capture.c - Direct DRM/KMS framebuffer capture
 * 
 * This is what makes us better than PipeWire/Steam Remote Play.
 * We read directly from the kernel's DRM subsystem, bypassing all
 * the compositor/portal nonsense that constantly breaks.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <dirent.h>
#include <stdarg.h>
#include <time.h>

/* DRM kernel headers */
#include <drm/drm.h>
#include <drm/drm_mode.h>

#ifndef DRM_MODE_CONNECTED
#define DRM_MODE_CONNECTED 1
#endif

static char last_error[256] = {0};

const char* rootstream_get_error(void) {
    return last_error;
}

static void set_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(last_error, sizeof(last_error), fmt, args);
    va_end(args);
}

/*
 * Detect available DRM devices
 * We iterate /dev/dri/card* and query their capabilities
 */
int rootstream_detect_displays(display_info_t *displays, int max_displays) {
    DIR *dir = opendir("/dev/dri");
    if (!dir) {
        set_error("Cannot open /dev/dri: %s", strerror(errno));
        return -1;
    }

    int count = 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) && count < max_displays) {
        if (strncmp(entry->d_name, "card", 4) != 0)
            continue;

        char path[256];
        if (snprintf(path, sizeof(path), "/dev/dri/%s", entry->d_name) >= (int)sizeof(path)) {
            continue;
        }
        
        int fd = open(path, O_RDWR | O_CLOEXEC);
        if (fd < 0)
            continue;

        /* Query DRM resources */
        struct drm_mode_card_res res = {0};
        if (ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res) < 0) {
            close(fd);
            continue;
        }

        /* Allocate arrays for resource IDs */
        uint32_t *connectors = calloc(res.count_connectors, sizeof(uint32_t));
        uint32_t *crtcs = calloc(res.count_crtcs, sizeof(uint32_t));
        uint32_t *fbs = calloc(res.count_fbs, sizeof(uint32_t));
        
        res.connector_id_ptr = (uint64_t)connectors;
        res.crtc_id_ptr = (uint64_t)crtcs;
        res.fb_id_ptr = (uint64_t)fbs;
        
        if (ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res) < 0) {
            free(connectors);
            free(crtcs);
            free(fbs);
            close(fd);
            continue;
        }

        /* Find first connected display */
        for (uint32_t i = 0; i < res.count_connectors; i++) {
            struct drm_mode_get_connector conn = {0};
            conn.connector_id = connectors[i];
            
            if (ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) < 0)
                continue;

            /* Only interested in connected displays */
            if (conn.connection != DRM_MODE_CONNECTED)
                continue;

            /* Get connector modes */
            struct drm_mode_modeinfo *modes = calloc(conn.count_modes, 
                                                     sizeof(struct drm_mode_modeinfo));
            conn.modes_ptr = (uint64_t)modes;
            
            if (ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) == 0 && conn.count_modes > 0) {
                /* Use first mode (usually native resolution) */
                displays[count].fd = fd;
                displays[count].connector_id = connectors[i];
                displays[count].crtc_id = conn.encoder_id;  /* Simplified */
                displays[count].width = modes[0].hdisplay;
                displays[count].height = modes[0].vdisplay;
                displays[count].refresh_rate = modes[0].vrefresh;
                
                /* Get connector name */
                if (snprintf(displays[count].name, sizeof(displays[count].name),
                             "%s-%u", entry->d_name, conn.connector_type) >=
                    (int)sizeof(displays[count].name)) {
                    strncpy(displays[count].name, "drm-unknown",
                            sizeof(displays[count].name) - 1);
                    displays[count].name[sizeof(displays[count].name) - 1] = '\0';
                }
                
                count++;
                free(modes);
                break;
            }
            
            free(modes);
        }

        free(connectors);
        free(crtcs);
        free(fbs);
        
        if (count == 0)
            close(fd);
    }

    closedir(dir);
    
    if (count == 0) {
        set_error("No active displays found");
        return -1;
    }
    
    return count;
}

/*
 * Select a display by index and attach it to the context.
 * This helper re-detects displays and retains the selected FD.
 */
int rootstream_select_display(rootstream_ctx_t *ctx, int display_index) {
    if (!ctx) {
        set_error("Display selection failed: NULL context");
        return -1;
    }

    if (display_index < 0) {
        set_error("Display selection failed: invalid index %d", display_index);
        return -1;
    }

    display_info_t displays[MAX_DISPLAYS];
    int num_displays = rootstream_detect_displays(displays, MAX_DISPLAYS);
    if (num_displays < 0) {
        return -1;
    }

    if (display_index >= num_displays) {
        for (int i = 0; i < num_displays; i++) {
            if (displays[i].fd >= 0) {
                close(displays[i].fd);
            }
        }
        set_error("Display selection failed: index %d out of range (0-%d)",
                  display_index, num_displays - 1);
        return -1;
    }

    ctx->display = displays[display_index];

    for (int i = 0; i < num_displays; i++) {
        if (i != display_index && displays[i].fd >= 0) {
            close(displays[i].fd);
        }
    }

    return 0;
}

/*
 * Initialize capture for a specific display
 */
int rootstream_capture_init(rootstream_ctx_t *ctx) {
    if (!ctx || ctx->display.fd < 0) {
        set_error("Invalid context or display not selected");
        return -1;
    }

    /* Get current framebuffer info */
    struct drm_mode_card_res res = {0};
    if (ioctl(ctx->display.fd, DRM_IOCTL_MODE_GETRESOURCES, &res) < 0) {
        set_error("Cannot get DRM resources: %s", strerror(errno));
        return -1;
    }

    if (res.count_fbs == 0) {
        set_error("No framebuffers available");
        return -1;
    }

    /* Get first framebuffer (active display) */
    uint32_t *fbs = calloc(res.count_fbs, sizeof(uint32_t));
    res.fb_id_ptr = (uint64_t)fbs;
    
    if (ioctl(ctx->display.fd, DRM_IOCTL_MODE_GETRESOURCES, &res) < 0) {
        free(fbs);
        set_error("Cannot get framebuffer IDs: %s", strerror(errno));
        return -1;
    }

    ctx->display.fb_id = fbs[0];
    free(fbs);

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

    printf("✓ DRM capture initialized: %dx%d @ %d Hz\n",
           ctx->display.width, ctx->display.height, ctx->display.refresh_rate);

    return 0;
}

/*
 * Capture a frame directly from GPU framebuffer
 * This is the magic - no compositor involved!
 */
int rootstream_capture_frame(rootstream_ctx_t *ctx, frame_buffer_t *frame) {
    if (!ctx || !frame) {
        set_error("Invalid arguments");
        return -1;
    }

    /* Get framebuffer info */
    struct drm_mode_fb_cmd fb_cmd = {0};
    fb_cmd.fb_id = ctx->display.fb_id;
    
    if (ioctl(ctx->display.fd, DRM_IOCTL_MODE_GETFB, &fb_cmd) < 0) {
        set_error("Cannot get framebuffer info: %s", strerror(errno));
        return -1;
    }

    /* Map the framebuffer into our address space */
    struct drm_mode_map_dumb map_req = {0};
    map_req.handle = fb_cmd.handle;
    
    if (ioctl(ctx->display.fd, DRM_IOCTL_MODE_MAP_DUMB, &map_req) < 0) {
        set_error("Cannot map framebuffer: %s", strerror(errno));
        return -1;
    }

    /* mmap the framebuffer */
    void *fb_map = mmap(0, fb_cmd.pitch * fb_cmd.height,
                        PROT_READ, MAP_SHARED,
                        ctx->display.fd, map_req.offset);
    
    if (fb_map == MAP_FAILED) {
        set_error("mmap failed: %s", strerror(errno));
        return -1;
    }

    /* Copy framebuffer data */
    memcpy(frame->data, fb_map, frame->size);
    frame->pitch = fb_cmd.pitch;
    
    /* Get timestamp */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    frame->timestamp = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;

    /* Cleanup */
    munmap(fb_map, fb_cmd.pitch * fb_cmd.height);

    ctx->frames_captured++;
    return 0;
}

void rootstream_capture_cleanup(rootstream_ctx_t *ctx) {
    if (!ctx)
        return;

    if (ctx->current_frame.data) {
        free(ctx->current_frame.data);
        ctx->current_frame.data = NULL;
    }

    if (ctx->display.fd >= 0) {
        close(ctx->display.fd);
        ctx->display.fd = -1;
    }
}
