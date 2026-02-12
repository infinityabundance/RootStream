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
#include <va/va_enc_h264.h>

typedef struct {
    VADisplay display;
    VAConfigID config_id;
    VAContextID context_id;
    VASurfaceID *surfaces;
    int num_surfaces;
    VABufferID coded_buf_id;

    /* H.264 parameter buffers */
    VABufferID seq_param_buf;
    VABufferID pic_param_buf;
    VABufferID slice_param_buf;

    int width;
    int height;
    int fps;
    uint32_t surface_index;  /* Current surface in ring buffer */
    uint32_t frame_num;       /* Frame counter for encoding */
} vaapi_ctx_t;

/* Forward declare from drm_capture.c */
extern const char* rootstream_get_error(void);

/* Forward declarations for NVENC */
extern int rootstream_encoder_init_nvenc(rootstream_ctx_t *ctx, codec_type_t codec);
extern int rootstream_encode_frame_nvenc(rootstream_ctx_t *ctx, frame_buffer_t *in,
                                         uint8_t *out, size_t *out_size);
extern void rootstream_encoder_cleanup_nvenc(rootstream_ctx_t *ctx);

/*
 * Detect if H.264 NAL stream contains an IDR (keyframe)
 *
 * Parses through NAL units looking for NAL type 5 (IDR slice).
 * Supports both Annex B (start code) format.
 *
 * NAL unit types:
 *   1 = Non-IDR slice
 *   5 = IDR slice (keyframe)
 *   7 = SPS
 *   8 = PPS
 *
 * @param data   Encoded H.264 data
 * @param size   Data size in bytes
 * @return       true if keyframe detected, false otherwise
 */
static bool detect_h264_keyframe(const uint8_t *data, size_t size) {
    if (!data || size < 5) {
        return false;
    }

    /* Search for NAL start codes (0x00 0x00 0x01 or 0x00 0x00 0x00 0x01) */
    for (size_t i = 0; i < size - 4; i++) {
        bool start_code_3 = (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x01);
        bool start_code_4 = (i + 4 < size && data[i] == 0x00 && data[i+1] == 0x00 &&
                            data[i+2] == 0x00 && data[i+3] == 0x01);

        if (start_code_3 || start_code_4) {
            /* Get NAL type from the byte following start code */
            size_t nal_byte_idx = start_code_4 ? i + 4 : i + 3;
            if (nal_byte_idx < size) {
                uint8_t nal_type = data[nal_byte_idx] & 0x1F;

                /* NAL type 5 = IDR (Instantaneous Decoder Refresh) = keyframe */
                if (nal_type == 5) {
                    return true;
                }
            }

            /* Skip past start code for next iteration */
            i += start_code_4 ? 3 : 2;
        }
    }

    return false;
}

/*
 * Detect if H.265/HEVC NAL stream contains an IDR (keyframe)
 *
 * HEVC NAL unit types for keyframes:
 *   19 = IDR_W_RADL
 *   20 = IDR_N_LP
 *   21 = CRA_NUT (Clean Random Access)
 *
 * @param data   Encoded H.265 data
 * @param size   Data size in bytes
 * @return       true if keyframe detected, false otherwise
 */
static bool detect_h265_keyframe(const uint8_t *data, size_t size) {
    if (!data || size < 5) {
        return false;
    }

    /* Search for NAL start codes */
    for (size_t i = 0; i < size - 4; i++) {
        bool start_code_3 = (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x01);
        bool start_code_4 = (i + 4 < size && data[i] == 0x00 && data[i+1] == 0x00 &&
                            data[i+2] == 0x00 && data[i+3] == 0x01);

        if (start_code_3 || start_code_4) {
            /* HEVC NAL header is 2 bytes, NAL type is in bits 1-6 of first byte */
            size_t nal_byte_idx = start_code_4 ? i + 4 : i + 3;
            if (nal_byte_idx < size) {
                uint8_t nal_type = (data[nal_byte_idx] >> 1) & 0x3F;

                /* IDR or CRA NAL types = keyframe */
                if (nal_type == 19 || nal_type == 20 || nal_type == 21) {
                    return true;
                }
            }

            i += start_code_4 ? 3 : 2;
        }
    }

    return false;
}

/*
 * Convert RGBA to NV12 colorspace
 *
 * NV12 format: Planar YUV 4:2:0
 * - Y plane: full resolution (width x height)
 * - UV plane: half resolution (width x height/2), interleaved U/V
 *
 * Uses ITU-R BT.709 color matrix (HD standard)
 */
static void rgba_to_nv12(const uint8_t *rgba, uint8_t *nv12_y, uint8_t *nv12_uv,
                         int width, int height, int y_stride, int uv_stride) {
    /* Convert each pixel */
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int rgba_idx = (y * width + x) * 4;
            int y_idx = y * y_stride + x;

            uint8_t r = rgba[rgba_idx];
            uint8_t g = rgba[rgba_idx + 1];
            uint8_t b = rgba[rgba_idx + 2];

            /* ITU-R BT.709 coefficients for Y */
            /* Y = 0.2126*R + 0.7152*G + 0.0722*B (scaled to 16-235 range) */
            int y_val = (66*r + 129*g + 25*b + 128) >> 8;
            nv12_y[y_idx] = (uint8_t)(y_val + 16);

            /* Sample U/V every 2x2 pixels (4:2:0 subsampling) */
            if ((y & 1) == 0 && (x & 1) == 0) {
                int uv_idx = (y/2) * uv_stride + (x & ~1);

                /* ITU-R BT.709 coefficients for U and V */
                /* U = -0.1146*R - 0.3854*G + 0.5*B */
                /* V =  0.5*R - 0.4542*G - 0.0458*B */
                int u_val = (-38*r - 74*g + 112*b + 128) >> 8;
                int v_val = (112*r - 94*g - 18*b + 128) >> 8;

                nv12_uv[uv_idx]   = (uint8_t)(u_val + 128);  /* U */
                nv12_uv[uv_idx+1] = (uint8_t)(v_val + 128);  /* V */
            }
        }
    }
}

/*
 * Check if VA-API encoder is available
 *
 * Attempts to open DRM device and check for VA-API support.
 * Returns true if VA-API H.264 encoding is available.
 */
bool rootstream_encoder_vaapi_available(void) {
    /* Try to open DRM device */
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        return false;
    }

    /* Try to get VA display */
    VADisplay display = vaGetDisplayDRM(drm_fd);
    if (!display) {
        close(drm_fd);
        return false;
    }

    /* Try to initialize VA-API */
    int major, minor;
    VAStatus status = vaInitialize(display, &major, &minor);
    if (status != VA_STATUS_SUCCESS) {
        close(drm_fd);
        return false;
    }

    /* Check for H.264 encoding support */
    int num_profiles = vaMaxNumProfiles(display);
    VAProfile *profiles_list = malloc(num_profiles * sizeof(VAProfile));
    if (!profiles_list) {
        vaTerminate(display);
        close(drm_fd);
        return false;
    }

    int actual_num_profiles;
    vaQueryConfigProfiles(display, profiles_list, &actual_num_profiles);

    bool supported = false;
    for (int i = 0; i < actual_num_profiles; i++) {
        if (profiles_list[i] == VAProfileH264High ||
            profiles_list[i] == VAProfileH264Main ||
            profiles_list[i] == VAProfileH264ConstrainedBaseline) {
            supported = true;
            break;
        }
    }

    free(profiles_list);
    vaTerminate(display);
    close(drm_fd);
    
    return supported;
}

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
    va->fps = ctx->display.refresh_rate ? ctx->display.refresh_rate : 60;

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
    size_t coded_buf_size = (size_t)ctx->display.width * ctx->display.height * 4;
    if (coded_buf_size > 64 * 1024 * 1024) {
        coded_buf_size = 64 * 1024 * 1024;
    }

    status = vaCreateBuffer(va->display, va->context_id,
                           VAEncCodedBufferType,
                           coded_buf_size,
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

    /* Initialize ring buffer and frame counter */
    va->surface_index = 0;
    va->frame_num = 0;

    ctx->encoder.type = ENCODER_VAAPI;
    ctx->encoder.codec = codec;
    ctx->encoder.hw_ctx = va;
    ctx->encoder.device_fd = drm_fd;
    ctx->encoder.max_output_size = coded_buf_size;
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

    /* Use ring buffer for better performance */
    VASurfaceID surface = va->surfaces[va->surface_index];
    va->surface_index = (va->surface_index + 1) % va->num_surfaces;

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

    /* Convert RGBA to NV12 with proper colorspace conversion */
    uint8_t *y_plane = (uint8_t*)va_data;
    uint8_t *uv_plane = y_plane + (image.pitches[0] * va->height);

    rgba_to_nv12(in->data, y_plane, uv_plane,
                 va->width, va->height,
                 image.pitches[0],  /* Y stride */
                 image.pitches[1]); /* UV stride */

    vaUnmapBuffer(va->display, image.buf);
    vaDestroyImage(va->display, image.image_id);

    /* Check if we should force a keyframe */
    bool force_idr = ctx->encoder.force_keyframe;
    if (force_idr) {
        ctx->encoder.force_keyframe = false;  /* Reset the flag */
    }

    /* Determine if this frame should be a keyframe */
    bool is_keyframe = force_idr || (va->frame_num % va->fps) == 0;

    /* Prepare H.264 encoding parameters */
    /* Sequence parameter buffer - global encoding settings */
    VAEncSequenceParameterBufferH264 seq_param = {0};
    seq_param.seq_parameter_set_id = 0;
    seq_param.level_idc = 41;  /* Level 4.1 (supports up to 1920x1080 @ 60fps) */
    seq_param.intra_period = va->fps;  /* I-frame every 1 second */
    seq_param.intra_idr_period = va->fps;
    seq_param.ip_period = 1;  /* No B-frames for low latency (I and P only) */
    seq_param.bits_per_second = ctx->encoder.bitrate;
    seq_param.max_num_ref_frames = 1;  /* Low latency - 1 reference frame */
    seq_param.picture_width_in_mbs = (va->width + 15) / 16;
    seq_param.picture_height_in_mbs = (va->height + 15) / 16;
    seq_param.seq_fields.bits.frame_mbs_only_flag = 1;  /* Progressive only */
    seq_param.time_scale = va->fps * 2;
    seq_param.num_units_in_tick = 1;
    seq_param.frame_cropping_flag = 0;
    seq_param.vui_parameters_present_flag = 0;

    /* Picture parameter buffer - per-frame settings */
    VAEncPictureParameterBufferH264 pic_param = {0};
    pic_param.CurrPic.picture_id = surface;
    pic_param.CurrPic.frame_idx = va->frame_num;
    pic_param.CurrPic.flags = 0;
    pic_param.CurrPic.TopFieldOrderCnt = va->frame_num * 2;
    pic_param.CurrPic.BottomFieldOrderCnt = va->frame_num * 2;

    /* Reference frame (for P-frames) */
    if (va->frame_num > 0) {
        pic_param.ReferenceFrames[0].picture_id = va->surfaces[(va->surface_index - 1 + va->num_surfaces) % va->num_surfaces];
        pic_param.ReferenceFrames[0].frame_idx = va->frame_num - 1;
        pic_param.ReferenceFrames[0].flags = 0;
        pic_param.ReferenceFrames[0].TopFieldOrderCnt = (va->frame_num - 1) * 2;
        pic_param.ReferenceFrames[0].BottomFieldOrderCnt = (va->frame_num - 1) * 2;
    } else {
        pic_param.ReferenceFrames[0].picture_id = VA_INVALID_SURFACE;
        pic_param.ReferenceFrames[0].flags = VA_PICTURE_H264_INVALID;
    }
    for (int i = 1; i < 16; i++) {
        pic_param.ReferenceFrames[i].picture_id = VA_INVALID_SURFACE;
        pic_param.ReferenceFrames[i].flags = VA_PICTURE_H264_INVALID;
    }

    pic_param.coded_buf = va->coded_buf_id;
    pic_param.pic_parameter_set_id = 0;
    pic_param.seq_parameter_set_id = 0;
    pic_param.last_picture = 0;
    pic_param.frame_num = va->frame_num;
    pic_param.pic_init_qp = 26;  /* Initial QP */
    pic_param.num_ref_idx_l0_active_minus1 = 0;
    pic_param.num_ref_idx_l1_active_minus1 = 0;
    pic_param.pic_fields.bits.idr_pic_flag = is_keyframe ? 1 : 0;
    pic_param.pic_fields.bits.reference_pic_flag = 1;
    pic_param.pic_fields.bits.entropy_coding_mode_flag = 1;  /* CABAC */
    pic_param.pic_fields.bits.deblocking_filter_control_present_flag = 1;

    /* Slice parameter buffer */
    VAEncSliceParameterBufferH264 slice_param = {0};
    slice_param.macroblock_address = 0;
    slice_param.num_macroblocks = seq_param.picture_width_in_mbs * seq_param.picture_height_in_mbs;
    slice_param.slice_type = is_keyframe ? 2 : 0;  /* 2=I-slice, 0=P-slice */
    slice_param.pic_parameter_set_id = 0;
    slice_param.idr_pic_id = va->frame_num / va->fps;
    slice_param.pic_order_cnt_lsb = (va->frame_num * 2) & 0xFF;
    slice_param.num_ref_idx_active_override_flag = 0;

    /* Create parameter buffers */
    status = vaCreateBuffer(va->display, va->context_id,
                           VAEncSequenceParameterBufferType,
                           sizeof(seq_param), 1, &seq_param, &va->seq_param_buf);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "Cannot create sequence parameter buffer: %d\n", status);
        return -1;
    }

    status = vaCreateBuffer(va->display, va->context_id,
                           VAEncPictureParameterBufferType,
                           sizeof(pic_param), 1, &pic_param, &va->pic_param_buf);
    if (status != VA_STATUS_SUCCESS) {
        vaDestroyBuffer(va->display, va->seq_param_buf);
        fprintf(stderr, "Cannot create picture parameter buffer: %d\n", status);
        return -1;
    }

    status = vaCreateBuffer(va->display, va->context_id,
                           VAEncSliceParameterBufferType,
                           sizeof(slice_param), 1, &slice_param, &va->slice_param_buf);
    if (status != VA_STATUS_SUCCESS) {
        vaDestroyBuffer(va->display, va->seq_param_buf);
        vaDestroyBuffer(va->display, va->pic_param_buf);
        fprintf(stderr, "Cannot create slice parameter buffer: %d\n", status);
        return -1;
    }

    /* Encode the frame */
    status = vaBeginPicture(va->display, va->context_id, surface);
    if (status != VA_STATUS_SUCCESS) {
        vaDestroyBuffer(va->display, va->seq_param_buf);
        vaDestroyBuffer(va->display, va->pic_param_buf);
        vaDestroyBuffer(va->display, va->slice_param_buf);
        fprintf(stderr, "vaBeginPicture failed: %d\n", status);
        return -1;
    }

    /* Render parameter buffers to encoder */
    status = vaRenderPicture(va->display, va->context_id, &va->seq_param_buf, 1);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "vaRenderPicture (seq) failed: %d\n", status);
        vaEndPicture(va->display, va->context_id);
        vaDestroyBuffer(va->display, va->seq_param_buf);
        vaDestroyBuffer(va->display, va->pic_param_buf);
        vaDestroyBuffer(va->display, va->slice_param_buf);
        return -1;
    }

    status = vaRenderPicture(va->display, va->context_id, &va->pic_param_buf, 1);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "vaRenderPicture (pic) failed: %d\n", status);
        vaEndPicture(va->display, va->context_id);
        vaDestroyBuffer(va->display, va->seq_param_buf);
        vaDestroyBuffer(va->display, va->pic_param_buf);
        vaDestroyBuffer(va->display, va->slice_param_buf);
        return -1;
    }

    status = vaRenderPicture(va->display, va->context_id, &va->slice_param_buf, 1);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "vaRenderPicture (slice) failed: %d\n", status);
        vaEndPicture(va->display, va->context_id);
        vaDestroyBuffer(va->display, va->seq_param_buf);
        vaDestroyBuffer(va->display, va->pic_param_buf);
        vaDestroyBuffer(va->display, va->slice_param_buf);
        return -1;
    }

    status = vaEndPicture(va->display, va->context_id);
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "vaEndPicture failed: %d\n", status);
        vaDestroyBuffer(va->display, va->seq_param_buf);
        vaDestroyBuffer(va->display, va->pic_param_buf);
        vaDestroyBuffer(va->display, va->slice_param_buf);
        return -1;
    }

    /* Clean up parameter buffers */
    vaDestroyBuffer(va->display, va->seq_param_buf);
    vaDestroyBuffer(va->display, va->pic_param_buf);
    vaDestroyBuffer(va->display, va->slice_param_buf);

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
    if (ctx->encoder.max_output_size > 0 && segment->size > ctx->encoder.max_output_size) {
        fprintf(stderr, "ERROR: Encoded frame too large (%u > %zu)\n",
                segment->size, ctx->encoder.max_output_size);
        vaUnmapBuffer(va->display, va->coded_buf_id);
        return -1;
    }
    *out_size = segment->size;
    memcpy(out, segment->buf, segment->size);

    /* Detect actual keyframe from NAL units */
    bool detected_keyframe;
    if (ctx->encoder.codec == CODEC_H265) {
        detected_keyframe = detect_h265_keyframe(out, *out_size);
    } else {
        detected_keyframe = detect_h264_keyframe(out, *out_size);
    }

    /* Store keyframe status in input frame for caller to access */
    in->is_keyframe = detected_keyframe;

    #ifdef DEBUG
    if (detected_keyframe) {
        printf("DEBUG: Encoded keyframe (frame %u, size %zu)\n",
               va->frame_num, *out_size);
    }
    #endif

    vaUnmapBuffer(va->display, va->coded_buf_id);

    /* Increment frame counter for next encode */
    va->frame_num++;
    ctx->frames_encoded++;
    return 0;
}

/*
 * Extended encode frame with explicit keyframe output
 *
 * Same as rootstream_encode_frame() but returns keyframe status via parameter.
 *
 * @param ctx         RootStream context
 * @param in          Input frame (RGBA)
 * @param out         Output buffer (encoded H.264/H.265)
 * @param out_size    Output: encoded size
 * @param is_keyframe Output: true if this is a keyframe
 * @return            0 on success, -1 on error
 */
int rootstream_encode_frame_ex(rootstream_ctx_t *ctx, frame_buffer_t *in,
                              uint8_t *out, size_t *out_size, bool *is_keyframe) {
    int result = rootstream_encode_frame(ctx, in, out, out_size);
    if (result == 0 && is_keyframe) {
        *is_keyframe = in->is_keyframe;
    }
    return result;
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
