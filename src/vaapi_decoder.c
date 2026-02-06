/*
 * vaapi_decoder.c - VA-API hardware decoding
 *
 * Hardware H.264 decoding for Intel and AMD GPUs.
 * Receives encoded H.264 stream and outputs NV12 frames.
 *
 * Architecture:
 * - Initialize VA-API with DRM display
 * - Create decode config for H.264
 * - Allocate surfaces for decoded frames
 * - Submit encoded data to decoder
 * - Map surfaces to get pixel data
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/* VA-API headers */
#include <va/va.h>
#include <va/va_drm.h>

typedef struct {
    VADisplay display;
    VAConfigID config_id;
    VAContextID context_id;
    VASurfaceID *surfaces;
    int num_surfaces;
    int current_surface;

    int drm_fd;
    int width;
    int height;

    /* Decode buffers */
    VABufferID pic_param_buf;
    VABufferID slice_param_buf;
    VABufferID slice_data_buf;
} vaapi_decoder_ctx_t;

/*
 * Initialize VA-API decoder
 *
 * @param ctx  RootStream context
 * @return     0 on success, -1 on error
 */
int rootstream_decoder_init(rootstream_ctx_t *ctx) {
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context for decoder init\n");
        return -1;
    }

    /* Open DRM device for VA-API */
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        fprintf(stderr, "ERROR: Cannot open render device: %s\n", strerror(errno));
        return -1;
    }

    /* Allocate decoder context */
    vaapi_decoder_ctx_t *dec = calloc(1, sizeof(vaapi_decoder_ctx_t));
    if (!dec) {
        close(drm_fd);
        fprintf(stderr, "ERROR: Cannot allocate decoder context\n");
        return -1;
    }
    dec->drm_fd = drm_fd;

    /* Initialize VA display */
    dec->display = vaGetDisplayDRM(drm_fd);
    if (!dec->display) {
        free(dec);
        close(drm_fd);
        fprintf(stderr, "ERROR: Cannot get VA display for decoder\n");
        return -1;
    }

    int major, minor;
    VAStatus status = vaInitialize(dec->display, &major, &minor);
    if (status != VA_STATUS_SUCCESS) {
        free(dec);
        close(drm_fd);
        fprintf(stderr, "ERROR: VA-API decoder initialization failed: %d\n", status);
        return -1;
    }

    printf("✓ VA-API decoder %d.%d initialized\n", major, minor);

    /* Check for H.264 and H.265 decoding support */
    int num_profiles = vaMaxNumProfiles(dec->display);
    VAProfile *profiles_list = malloc(num_profiles * sizeof(VAProfile));
    int actual_num_profiles;

    vaQueryConfigProfiles(dec->display, profiles_list, &actual_num_profiles);

    bool h264_decode_supported = false;
    bool h265_decode_supported = false;
    VAProfile selected_profile = VAProfileH264Main;

    for (int i = 0; i < actual_num_profiles; i++) {
        if (profiles_list[i] == VAProfileH264High ||
            profiles_list[i] == VAProfileH264Main) {
            h264_decode_supported = true;
            selected_profile = profiles_list[i];
        }
        if (profiles_list[i] == VAProfileHEVCMain) {
            h265_decode_supported = true;
            /* Will switch to HEVC profile if needed */
        }
    }

    free(profiles_list);

    /* Use H.264 as default for now (codec negotiation in future) */
    codec_type_t codec = ctx->encoder.codec;  /* Match encoder codec */
    const char *codec_name;

    if (codec == CODEC_H265) {
        if (!h265_decode_supported) {
            fprintf(stderr, "ERROR: H.265 decode not supported by GPU\n");
            vaTerminate(dec->display);
            free(dec);
            close(drm_fd);
            return -1;
        }
        selected_profile = VAProfileHEVCMain;
        codec_name = "H.265/HEVC";
    } else {
        if (!h264_decode_supported) {
            fprintf(stderr, "ERROR: H.264 decode not supported by GPU\n");
            vaTerminate(dec->display);
            free(dec);
            close(drm_fd);
            return -1;
        }
        codec_name = "H.264";
    }

    printf("INFO: Using %s decoder\n", codec_name);

    printf("✓ H.264 decode profile supported\n");

    /* Create decode config */
    VAConfigAttrib attrib;
    attrib.type = VAConfigAttribRTFormat;
    vaGetConfigAttributes(dec->display, selected_profile, VAEntrypointVLD, &attrib, 1);

    if (!(attrib.value & VA_RT_FORMAT_YUV420)) {
        fprintf(stderr, "ERROR: YUV420 format not supported\n");
        vaTerminate(dec->display);
        free(dec);
        close(drm_fd);
        return -1;
    }

    status = vaCreateConfig(dec->display, selected_profile, VAEntrypointVLD,
                           &attrib, 1, &dec->config_id);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "ERROR: Cannot create decode config: %d\n", status);
        vaTerminate(dec->display);
        free(dec);
        close(drm_fd);
        return -1;
    }

    /* Use default resolution, will be updated on first frame */
    dec->width = 1920;
    dec->height = 1080;
    dec->num_surfaces = 8;  /* Buffer pool for smooth decoding */

    /* Create surfaces for decoded frames */
    dec->surfaces = malloc(dec->num_surfaces * sizeof(VASurfaceID));
    status = vaCreateSurfaces(dec->display, VA_RT_FORMAT_YUV420,
                             dec->width, dec->height,
                             dec->surfaces, dec->num_surfaces,
                             NULL, 0);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "ERROR: Cannot create decode surfaces: %d\n", status);
        vaDestroyConfig(dec->display, dec->config_id);
        vaTerminate(dec->display);
        free(dec->surfaces);
        free(dec);
        close(drm_fd);
        return -1;
    }

    /* Create decode context */
    status = vaCreateContext(dec->display, dec->config_id,
                            dec->width, dec->height, VA_PROGRESSIVE,
                            dec->surfaces, dec->num_surfaces,
                            &dec->context_id);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "ERROR: Cannot create decode context: %d\n", status);
        vaDestroySurfaces(dec->display, dec->surfaces, dec->num_surfaces);
        vaDestroyConfig(dec->display, dec->config_id);
        vaTerminate(dec->display);
        free(dec->surfaces);
        free(dec);
        close(drm_fd);
        return -1;
    }

    dec->current_surface = 0;

    /* Store decoder context in encoder context (reusing field) */
    ctx->encoder.hw_ctx = dec;
    ctx->encoder.type = ENCODER_VAAPI;  /* Reuse encoder type field */

    printf("✓ VA-API decoder ready: %dx%d with %d surfaces\n",
           dec->width, dec->height, dec->num_surfaces);

    return 0;
}

/*
 * Decode a single H.264 frame
 *
 * @param ctx      RootStream context
 * @param in       Input H.264 encoded data
 * @param in_size  Input data size
 * @param out      Output frame buffer (NV12 format)
 * @return         0 on success, -1 on error
 *
 * Note: This is a simplified decoder that assumes complete frames.
 * Production implementation would need proper H.264 bitstream parsing.
 */
int rootstream_decode_frame(rootstream_ctx_t *ctx,
                           const uint8_t *in, size_t in_size,
                           frame_buffer_t *out) {
    if (!ctx || !in || !out) {
        fprintf(stderr, "ERROR: Invalid arguments to decode_frame\n");
        return -1;
    }

    vaapi_decoder_ctx_t *dec = (vaapi_decoder_ctx_t*)ctx->encoder.hw_ctx;
    if (!dec) {
        fprintf(stderr, "ERROR: Decoder not initialized\n");
        return -1;
    }

    /* Select next surface from pool */
    VASurfaceID surface = dec->surfaces[dec->current_surface];
    dec->current_surface = (dec->current_surface + 1) % dec->num_surfaces;

    VAStatus status;

    /* Begin picture */
    status = vaBeginPicture(dec->display, dec->context_id, surface);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "ERROR: vaBeginPicture failed: %d\n", status);
        return -1;
    }

    /* Create slice data buffer with encoded data */
    VABufferID slice_data_buf;
    status = vaCreateBuffer(dec->display, dec->context_id,
                           VASliceDataBufferType, in_size, 1,
                           (void*)in, &slice_data_buf);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "ERROR: Cannot create slice data buffer: %d\n", status);
        vaEndPicture(dec->display, dec->context_id);
        return -1;
    }

    /* Render the slice data */
    status = vaRenderPicture(dec->display, dec->context_id,
                            &slice_data_buf, 1);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "ERROR: vaRenderPicture failed: %d\n", status);
        vaDestroyBuffer(dec->display, slice_data_buf);
        vaEndPicture(dec->display, dec->context_id);
        return -1;
    }

    /* End picture (submit for decoding) */
    status = vaEndPicture(dec->display, dec->context_id);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "ERROR: vaEndPicture failed: %d\n", status);
        vaDestroyBuffer(dec->display, slice_data_buf);
        return -1;
    }

    /* Wait for decode to complete */
    status = vaSyncSurface(dec->display, surface);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "ERROR: vaSyncSurface failed: %d\n", status);
        vaDestroyBuffer(dec->display, slice_data_buf);
        return -1;
    }

    /* Map surface to get decoded pixels (NV12 format) */
    VAImage image;
    status = vaDeriveImage(dec->display, surface, &image);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "ERROR: vaDeriveImage failed: %d\n", status);
        vaDestroyBuffer(dec->display, slice_data_buf);
        return -1;
    }

    /* Map image to CPU memory */
    void *mapped_data;
    status = vaMapBuffer(dec->display, image.buf, &mapped_data);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "ERROR: vaMapBuffer failed: %d\n", status);
        vaDestroyImage(dec->display, image.image_id);
        vaDestroyBuffer(dec->display, slice_data_buf);
        return -1;
    }

    /* Copy decoded frame to output buffer */
    out->width = dec->width;
    out->height = dec->height;
    out->pitch = image.pitches[0];
    out->format = image.format.fourcc;
    out->size = image.data_size;
    out->timestamp = get_timestamp_us();

    /* Allocate output buffer if needed */
    if (!out->data || out->capacity < out->size) {
        uint8_t *new_buf = realloc(out->data, out->size);
        if (!new_buf) {
            fprintf(stderr, "ERROR: Cannot allocate output buffer\n");
            vaUnmapBuffer(dec->display, image.buf);
            vaDestroyImage(dec->display, image.image_id);
            vaDestroyBuffer(dec->display, slice_data_buf);
            return -1;
        }
        out->data = new_buf;
        out->capacity = out->size;
    }

    /* Copy pixel data */
    memcpy(out->data, mapped_data, out->size);

    /* Cleanup */
    vaUnmapBuffer(dec->display, image.buf);
    vaDestroyImage(dec->display, image.image_id);
    vaDestroyBuffer(dec->display, slice_data_buf);

    return 0;
}

/*
 * Cleanup decoder resources
 */
void rootstream_decoder_cleanup(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->encoder.hw_ctx) {
        return;
    }

    vaapi_decoder_ctx_t *dec = (vaapi_decoder_ctx_t*)ctx->encoder.hw_ctx;

    /* Destroy decode context */
    if (dec->context_id) {
        vaDestroyContext(dec->display, dec->context_id);
    }

    /* Destroy surfaces */
    if (dec->surfaces) {
        vaDestroySurfaces(dec->display, dec->surfaces, dec->num_surfaces);
        free(dec->surfaces);
    }

    /* Destroy config */
    if (dec->config_id) {
        vaDestroyConfig(dec->display, dec->config_id);
    }

    /* Terminate VA-API */
    if (dec->display) {
        vaTerminate(dec->display);
    }

    /* Close DRM device */
    if (dec->drm_fd >= 0) {
        close(dec->drm_fd);
    }

    free(dec);
    ctx->encoder.hw_ctx = NULL;

    printf("✓ Decoder cleanup complete\n");
}
