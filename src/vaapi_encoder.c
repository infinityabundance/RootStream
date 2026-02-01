/*
 * vaapi_encoder.c - VA-API hardware encoding
 * 
 * Hardware H.264 encoding for Intel and AMD GPUs.
 * Zero-copy from DRM buffer to encoder when possible.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

/* VA-API headers (typically in /usr/include/va) */
#include <va/va.h>
#include <va/va_drm.h>

typedef struct {
    VADisplay display;
    VAConfigID config_id;
    VAContextID context_id;
    VASurfaceID *surfaces;
    int num_surfaces;
    VABufferID coded_buf_id;
    
    int width;
    int height;
    int fps;
} vaapi_ctx_t;

/* Forward declare from drm_capture.c */
extern const char* rootstream_get_error(void);

/* Forward declarations for NVENC */
extern int rootstream_encoder_init_nvenc(rootstream_ctx_t *ctx, codec_type_t codec);
extern int rootstream_encode_frame_nvenc(rootstream_ctx_t *ctx, frame_buffer_t *in,
                                         uint8_t *out, size_t *out_size);
extern void rootstream_encoder_cleanup_nvenc(rootstream_ctx_t *ctx);

/*
 * Initialize encoder (routes to VA-API or NVENC)
 */
int rootstream_encoder_init(rootstream_ctx_t *ctx, encoder_type_t type, codec_type_t codec) {
    if (!ctx) {
        fprintf(stderr, "Invalid context\n");
        return -1;
    }

    /* Route to NVENC if requested */
    if (type == ENCODER_NVENC) {
        return rootstream_encoder_init_nvenc(ctx, codec);
    }

    if (type != ENCODER_VAAPI) {
        fprintf(stderr, "Unsupported encoder type: %d\n", type);
        return -1;
    }

    /* Open DRM device for VA-API */
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        fprintf(stderr, "Cannot open render device: %s\n", strerror(errno));
        return -1;
    }

    /* Allocate VA-API context */
    vaapi_ctx_t *va = calloc(1, sizeof(vaapi_ctx_t));
    if (!va) {
        close(drm_fd);
        fprintf(stderr, "Cannot allocate VA-API context\n");
        return -1;
    }

    /* Initialize VA display */
    va->display = vaGetDisplayDRM(drm_fd);
    if (!va->display) {
        free(va);
        close(drm_fd);
        fprintf(stderr, "Cannot get VA display\n");
        return -1;
    }

    int major, minor;
    VAStatus status = vaInitialize(va->display, &major, &minor);
    if (status != VA_STATUS_SUCCESS) {
        free(va);
        close(drm_fd);
        fprintf(stderr, "VA-API initialization failed: %d\n", status);
        return -1;
    }

    printf("✓ VA-API %d.%d initialized\n", major, minor);

    /* Check for codec support */
    int num_profiles = vaMaxNumProfiles(va->display);
    VAProfile *profiles_list = malloc(num_profiles * sizeof(VAProfile));
    int actual_num_profiles;

    vaQueryConfigProfiles(va->display, profiles_list, &actual_num_profiles);

    bool codec_supported = false;
    VAProfile selected_profile;
    const char *codec_name;

    if (codec == CODEC_H265) {
        /* Check for H.265/HEVC support */
        for (int i = 0; i < actual_num_profiles; i++) {
            if (profiles_list[i] == VAProfileHEVCMain) {
                codec_supported = true;
                selected_profile = VAProfileHEVCMain;
                codec_name = "H.265/HEVC";
                break;
            }
        }
    } else {
        /* Check for H.264 support */
        for (int i = 0; i < actual_num_profiles; i++) {
            if (profiles_list[i] == VAProfileH264Main ||
                profiles_list[i] == VAProfileH264High) {
                codec_supported = true;
                selected_profile = VAProfileH264High;
                codec_name = "H.264";
                break;
            }
        }
    }
    free(profiles_list);

    if (!codec_supported) {
        vaTerminate(va->display);
        free(va);
        close(drm_fd);
        fprintf(stderr, "ERROR: %s encoding not supported\n",
                codec == CODEC_H265 ? "H.265" : "H.264");
        return -1;
    }

    /* Create encoding config */
    VAConfigAttrib attrib;
    attrib.type = VAConfigAttribRateControl;
    attrib.value = VA_RC_CBR;  /* Constant bitrate */

    status = vaCreateConfig(va->display, selected_profile,
                           VAEntrypointEncSlice,
                           &attrib, 1, &va->config_id);
    if (status != VA_STATUS_SUCCESS) {
        vaTerminate(va->display);
        free(va);
        close(drm_fd);
        fprintf(stderr, "Cannot create VA config: %d\n", status);
        return -1;
    }

    /* Set encoding parameters */
    va->width = ctx->display.width;
    va->height = ctx->display.height;
    va->fps = ctx->display.refresh_rate;

    /* Create surfaces (render targets) */
    va->num_surfaces = 4;  /* Ring buffer */
    va->surfaces = malloc(va->num_surfaces * sizeof(VASurfaceID));
    
    status = vaCreateSurfaces(va->display, VA_RT_FORMAT_YUV420,
                             va->width, va->height,
                             va->surfaces, va->num_surfaces,
                             NULL, 0);
    if (status != VA_STATUS_SUCCESS) {
        vaDestroyConfig(va->display, va->config_id);
        vaTerminate(va->display);
        free(va->surfaces);
        free(va);
        close(drm_fd);
        fprintf(stderr, "Cannot create VA surfaces: %d\n", status);
        return -1;
    }

    /* Create encoding context */
    status = vaCreateContext(va->display, va->config_id,
                            va->width, va->height, VA_PROGRESSIVE,
                            va->surfaces, va->num_surfaces,
                            &va->context_id);
    if (status != VA_STATUS_SUCCESS) {
        vaDestroySurfaces(va->display, va->surfaces, va->num_surfaces);
        vaDestroyConfig(va->display, va->config_id);
        vaTerminate(va->display);
        free(va->surfaces);
        free(va);
        close(drm_fd);
        fprintf(stderr, "Cannot create VA context: %d\n", status);
        return -1;
    }

    /* Create coded buffer (output) */
    status = vaCreateBuffer(va->display, va->context_id,
                           VAEncCodedBufferType,
                           ctx->display.width * ctx->display.height,
                           1, NULL, &va->coded_buf_id);
    if (status != VA_STATUS_SUCCESS) {
        vaDestroyContext(va->display, va->context_id);
        vaDestroySurfaces(va->display, va->surfaces, va->num_surfaces);
        vaDestroyConfig(va->display, va->config_id);
        vaTerminate(va->display);
        free(va->surfaces);
        free(va);
        close(drm_fd);
        fprintf(stderr, "Cannot create coded buffer: %d\n", status);
        return -1;
    }

    ctx->encoder.type = ENCODER_VAAPI;
    ctx->encoder.codec = codec;
    ctx->encoder.hw_ctx = va;
    ctx->encoder.device_fd = drm_fd;
    if (ctx->encoder.bitrate == 0) {
        ctx->encoder.bitrate = 10000000;  /* 10 Mbps default */
    }
    ctx->encoder.framerate = va->fps;
    ctx->encoder.low_latency = true;

    printf("✓ VA-API %s encoder ready: %dx%d @ %d fps, %d kbps\n",
           codec_name, va->width, va->height, va->fps, ctx->encoder.bitrate / 1000);

    return 0;
}

/*
 * Encode a frame (routes to VA-API or NVENC)
 */
int rootstream_encode_frame(rootstream_ctx_t *ctx, frame_buffer_t *in,
                           uint8_t *out, size_t *out_size) {
    if (!ctx || !in || !out || !out_size) {
        fprintf(stderr, "Invalid arguments\n");
        return -1;
    }

    /* Route to NVENC if active */
    if (ctx->encoder.type == ENCODER_NVENC) {
        return rootstream_encode_frame_nvenc(ctx, in, out, out_size);
    }

    vaapi_ctx_t *va = (vaapi_ctx_t*)ctx->encoder.hw_ctx;
    if (!va) {
        fprintf(stderr, "Encoder not initialized\n");
        return -1;
    }

    /* Use first surface from ring buffer (simplified) */
    VASurfaceID surface = va->surfaces[0];

    /* Upload frame to surface */
    VAImage image;
    VAStatus status = vaDeriveImage(va->display, surface, &image);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "Cannot derive VA image: %d\n", status);
        return -1;
    }

    void *va_data;
    status = vaMapBuffer(va->display, image.buf, &va_data);
    if (status != VA_STATUS_SUCCESS) {
        vaDestroyImage(va->display, image.image_id);
        fprintf(stderr, "Cannot map VA buffer: %d\n", status);
        return -1;
    }

    /* Convert RGBA to NV12 (simplified - proper conversion needed) */
    /* This is a placeholder - real implementation needs colorspace conversion */
    memcpy(va_data, in->data, in->size);

    vaUnmapBuffer(va->display, image.buf);
    vaDestroyImage(va->display, image.image_id);

    /* Encode the frame */
    status = vaBeginPicture(va->display, va->context_id, surface);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "vaBeginPicture failed: %d\n", status);
        return -1;
    }

    /* TODO: Add sequence, picture, and slice parameter buffers */
    /* This is simplified - full implementation needs proper H.264 params */

    status = vaEndPicture(va->display, va->context_id);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "vaEndPicture failed: %d\n", status);
        return -1;
    }

    /* Wait for encoding to complete */
    status = vaSyncSurface(va->display, surface);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "vaSyncSurface failed: %d\n", status);
        return -1;
    }

    /* Get encoded data */
    VACodedBufferSegment *segment;
    status = vaMapBuffer(va->display, va->coded_buf_id, (void**)&segment);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "Cannot map coded buffer: %d\n", status);
        return -1;
    }

    /* Copy encoded data */
    *out_size = segment->size;
    memcpy(out, segment->buf, segment->size);

    vaUnmapBuffer(va->display, va->coded_buf_id);

    ctx->frames_encoded++;
    return 0;
}

void rootstream_encoder_cleanup(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->encoder.hw_ctx)
        return;

    /* Route to NVENC cleanup if active */
    if (ctx->encoder.type == ENCODER_NVENC) {
        rootstream_encoder_cleanup_nvenc(ctx);
        return;
    }

    vaapi_ctx_t *va = (vaapi_ctx_t*)ctx->encoder.hw_ctx;

    vaDestroyBuffer(va->display, va->coded_buf_id);
    vaDestroyContext(va->display, va->context_id);
    vaDestroySurfaces(va->display, va->surfaces, va->num_surfaces);
    vaDestroyConfig(va->display, va->config_id);
    vaTerminate(va->display);
    
    close(ctx->encoder.device_fd);
    free(va->surfaces);
    free(va);
    
    ctx->encoder.hw_ctx = NULL;
}
