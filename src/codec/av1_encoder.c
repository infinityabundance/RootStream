/*
 * av1_encoder.c — AV1 encoder implementation (libaom / SVT-AV1 / HW)
 *
 * IMPLEMENTATION STRATEGY
 * -----------------------
 * This file provides:
 *   1. A compile-time dispatch table: each HAVE_* backend compiles
 *      its own static init/encode/cleanup functions; the dispatch
 *      table maps av1_backend_t → function pointers.
 *   2. av1_encoder_detect_backend() probes which backends are live at
 *      runtime (library present, hardware device opened successfully).
 *   3. av1_encoder_create() selects the best live backend and populates
 *      the context.
 *
 * When NONE of the HAVE_* macros are defined this file compiles to a
 * stub that reports available=false, allowing the build to succeed on
 * systems without AV1 libraries while the fallback chain routes sessions
 * to H.265 or H.264 instead.
 *
 * LIBAOM NOTES
 * ------------
 * libaom uses an internal thread pool sized to min(cpu_count, tile_count).
 * For low-latency streaming: set deadline=AOM_DL_REALTIME, lag_in_frames=0,
 * error_resilient=1 (each frame decodable independently).
 *
 * SVT-AV1 NOTES
 * -------------
 * SVT-AV1 achieves ~10x faster encoding than libaom at comparable quality
 * by using a hierarchical-B pipeline.  For streaming: ENC_MODE=8 (fast),
 * PRED_STRUCTURE=LOW_DELAY_P (no B-frames, minimal latency).
 *
 * VA-API AV1 NOTES
 * ----------------
 * VA-API AV1 encoding requires Intel Iris Xe (DG2) or AMD RDNA3+.  The
 * av1_vaapi path opens /dev/dri/renderD128 and queries
 * vaQueryConfigEntrypoints() for VAEntrypointEncSlice with VAProfileAV1Profile0.
 * If the query fails av1_encoder_detect_backend() falls through to SVT-AV1.
 *
 * NVENC AV1 NOTES
 * ---------------
 * NVENC AV1 requires NVIDIA RTX 4000 series (Ada Lovelace) or newer.
 * Detection: cuDeviceGetAttribute(CU_DEVICE_ATTRIBUTE_…).
 */

#include "av1_encoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Conditional includes ─────────────────────────────────────────── */
#ifdef HAVE_LIBAOM
#include <aom/aom_codec.h>
#include <aom/aom_encoder.h>
#include <aom/aomcx.h>
#endif

#ifdef HAVE_SVT_AV1
#include <svt-av1/EbSvtAv1Enc.h>
#endif

#ifdef HAVE_AV1_VAAPI
#include <va/va.h>
#include <va/va_drm.h>
#endif

/* ── Internal context ─────────────────────────────────────────────── */

struct av1_encoder_ctx_s {
    av1_backend_t backend;       /**< Backend actually in use              */
    av1_encoder_config_t config; /**< Copy of caller-supplied config       */
    bool need_kf;                /**< Request keyframe on next encode call  */

    /* Backend-specific sub-contexts — only one is non-NULL at a time. */
#ifdef HAVE_LIBAOM
    aom_codec_ctx_t *aom_ctx;
    aom_image_t aom_img;
#endif
#ifdef HAVE_SVT_AV1
    EbComponentType *svt_handle;
    EbBufferHeaderType *svt_in;
#endif
    /* VAAPI and NVENC contexts are opaque void* to avoid polluting the
     * header with platform-specific types. */
    void *hw_ctx;

    /* Statistics tracking */
    uint64_t frames_encoded;
    uint64_t bytes_out_total;
};

/* ── Capability detection ─────────────────────────────────────────── */

bool av1_encoder_available(void) {
    return av1_encoder_detect_backend() != AV1_BACKEND_NONE;
}

av1_backend_t av1_encoder_detect_backend(void) {
    /* Probe in priority order: hardware first, then software. */

#ifdef HAVE_AV1_VAAPI
    /* Try to open DRM render node and query AV1 encode capability */
    {
        int drm_fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
        if (drm_fd >= 0) {
            VADisplay dpy = vaGetDisplayDRM(drm_fd);
            int major, minor;
            if (vaInitialize(dpy, &major, &minor) == VA_STATUS_SUCCESS) {
                /* Check for AV1 encode entry point */
                VAEntrypoint eps[10];
                int n_eps = 0;
                VAStatus s = vaQueryConfigEntrypoints(dpy, VAProfileAV1Profile0, eps, &n_eps);
                bool found = false;
                if (s == VA_STATUS_SUCCESS) {
                    for (int i = 0; i < n_eps; i++) {
                        if (eps[i] == VAEntrypointEncSlice) {
                            found = true;
                            break;
                        }
                    }
                }
                vaTerminate(dpy);
                close(drm_fd);
                if (found)
                    return AV1_BACKEND_VAAPI;
            } else {
                close(drm_fd);
            }
        }
    }
#endif

#ifdef HAVE_AV1_NVENC
    /* Check CUDA device capabilities for AV1 NVENC.
     * Requires Ada Lovelace (RTX 4000+) GPU. */
    {
        /* Lightweight check: query CUDA driver without initialising NVENC fully */
        int device_count = 0;
        if (cuDeviceGetCount(&device_count) == CUDA_SUCCESS && device_count > 0) {
            /* Full NVENC capability check would go here */
            return AV1_BACKEND_NVENC;
        }
    }
#endif

#ifdef HAVE_SVT_AV1
    /* SVT-AV1: just check if the library initialises cleanly */
    {
        EbComponentType *handle = NULL;
        EbSvtAv1EncConfiguration cfg = {0};
        if (svt_av1_enc_init_handle(&handle, NULL, &cfg) == EB_ErrorNone) {
            svt_av1_enc_deinit_handle(handle);
            return AV1_BACKEND_SVT;
        }
    }
#endif

#ifdef HAVE_LIBAOM
    /* libaom: always available when compiled in */
    return AV1_BACKEND_LIBAOM;
#endif

    return AV1_BACKEND_NONE;
}

/* ── Lifecycle ────────────────────────────────────────────────────── */

av1_encoder_ctx_t *av1_encoder_create(const av1_encoder_config_t *config) {
    if (!config || config->width <= 0 || config->height <= 0)
        return NULL;

    av1_backend_t backend = config->preferred_backend;
    if (backend == AV1_BACKEND_NONE) {
        /* Auto-detect best available backend */
        backend = av1_encoder_detect_backend();
        if (backend == AV1_BACKEND_NONE) {
            fprintf(stderr,
                    "av1_encoder: no AV1 backend available — "
                    "install libaom or SVT-AV1\n");
            return NULL;
        }
    }

    av1_encoder_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx)
        return NULL;

    ctx->backend = backend;
    ctx->config = *config;
    ctx->need_kf = true; /* first frame is always a keyframe */

    switch (backend) {
#ifdef HAVE_LIBAOM
        case AV1_BACKEND_LIBAOM: {
            /* Configure libaom for real-time streaming.
             *
             * Key parameters for low-latency streaming:
             *   deadline = AOM_DL_REALTIME: use fast encode path
             *   lag_in_frames = 0: no lookahead (zero additional latency)
             *   error_resilient = 1: each frame independently decodable
             *     (essential: if a packet is dropped, the next keyframe can
             *      be decoded without the preceding frames)
             *   cpu_used = 8: fastest preset (acceptable quality for 60fps)
             */
            aom_codec_enc_cfg_t aom_cfg;
            aom_codec_iface_t *iface = aom_codec_av1_cx();

            aom_codec_enc_config_default(iface, &aom_cfg, AOM_USAGE_REALTIME);
            aom_cfg.g_w = (unsigned)config->width;
            aom_cfg.g_h = (unsigned)config->height;
            aom_cfg.g_timebase.num = 1;
            aom_cfg.g_timebase.den = config->fps > 0 ? config->fps : 60;
            aom_cfg.rc_target_bitrate = config->bitrate_kbps > 0 ? config->bitrate_kbps : 4000;
            aom_cfg.g_lag_in_frames = 0;
            aom_cfg.g_error_resilient = AOM_ERROR_RESILIENT_DEFAULT;
            aom_cfg.g_threads = 4; /* use 4 encoder threads by default */
            aom_cfg.tile_columns = config->tile_columns > 0 ? config->tile_columns : 2;
            aom_cfg.tile_rows = config->tile_rows > 0 ? config->tile_rows : 1;

            ctx->aom_ctx = calloc(1, sizeof(aom_codec_ctx_t));
            if (!ctx->aom_ctx) {
                free(ctx);
                return NULL;
            }

            aom_codec_err_t err = aom_codec_enc_init(ctx->aom_ctx, iface, &aom_cfg, 0);
            if (err != AOM_CODEC_OK) {
                fprintf(stderr, "av1_encoder: libaom init failed: %s\n",
                        aom_codec_err_to_string(err));
                free(ctx->aom_ctx);
                free(ctx);
                return NULL;
            }

            /* cpu_used=8 = fastest preset for real-time streaming */
            aom_codec_control(ctx->aom_ctx, AOME_SET_CPUUSED, 8);

            /* Initialise the input image descriptor */
            aom_img_alloc(&ctx->aom_img, AOM_IMG_FMT_I420, (unsigned)config->width,
                          (unsigned)config->height, 1);
            break;
        }
#endif /* HAVE_LIBAOM */

#ifdef HAVE_SVT_AV1
        case AV1_BACKEND_SVT: {
            /* SVT-AV1 configuration for streaming.
             *
             * EncMode 8 = fast preset optimised for real-time streaming.
             * PredStructure = SVT_AV1_PRED_LOW_DELAY_P: P-frames only,
             *   no B-frames, minimises encode latency.
             */
            EbSvtAv1EncConfiguration svt_cfg = {0};
            if (svt_av1_enc_init_handle(&ctx->svt_handle, NULL, &svt_cfg) != EB_ErrorNone) {
                free(ctx);
                return NULL;
            }
            svt_cfg.enc_mode = 8;
            svt_cfg.source_width = (uint32_t)config->width;
            svt_cfg.source_height = (uint32_t)config->height;
            svt_cfg.frame_rate = config->fps > 0 ? (uint32_t)config->fps : 60;
            svt_cfg.target_bit_rate =
                config->bitrate_kbps > 0 ? config->bitrate_kbps * 1000 : 4000000;
            svt_cfg.pred_structure = SVT_AV1_PRED_LOW_DELAY_P;
            svt_cfg.low_latency = config->low_latency ? 1 : 0;
            svt_cfg.tile_columns = config->tile_columns > 0 ? config->tile_columns : 1;
            svt_cfg.tile_rows = config->tile_rows > 0 ? config->tile_rows : 1;

            if (svt_av1_enc_set_parameter(ctx->svt_handle, &svt_cfg) != EB_ErrorNone ||
                svt_av1_enc_init(ctx->svt_handle) != EB_ErrorNone) {
                svt_av1_enc_deinit_handle(ctx->svt_handle);
                free(ctx);
                return NULL;
            }

            ctx->svt_in = calloc(1, sizeof(EbBufferHeaderType));
            if (!ctx->svt_in) {
                svt_av1_enc_deinit(ctx->svt_handle);
                svt_av1_enc_deinit_handle(ctx->svt_handle);
                free(ctx);
                return NULL;
            }
            break;
        }
#endif /* HAVE_SVT_AV1 */

        default:
            /* VAAPI and NVENC backends are initialised via the hw_ctx pointer.
             * Full implementation follows the same pattern as vaapi_encoder.c. */
            fprintf(stderr, "av1_encoder: backend %d not fully implemented yet\n", (int)backend);
            free(ctx);
            return NULL;
    }

    fprintf(stderr, "av1_encoder: initialised backend=%d (%s) %dx%d @ %dfps\n", (int)backend,
            backend == AV1_BACKEND_LIBAOM  ? "libaom"
            : backend == AV1_BACKEND_SVT   ? "svt-av1"
            : backend == AV1_BACKEND_VAAPI ? "vaapi"
            : backend == AV1_BACKEND_NVENC ? "nvenc"
                                           : "unknown",
            config->width, config->height, config->fps);

    return ctx;
}

void av1_encoder_destroy(av1_encoder_ctx_t *ctx) {
    if (!ctx)
        return;

#ifdef HAVE_LIBAOM
    if (ctx->backend == AV1_BACKEND_LIBAOM && ctx->aom_ctx) {
        aom_codec_destroy(ctx->aom_ctx);
        aom_img_free(&ctx->aom_img);
        free(ctx->aom_ctx);
    }
#endif

#ifdef HAVE_SVT_AV1
    if (ctx->backend == AV1_BACKEND_SVT && ctx->svt_handle) {
        svt_av1_enc_deinit(ctx->svt_handle);
        svt_av1_enc_deinit_handle(ctx->svt_handle);
        free(ctx->svt_in);
    }
#endif

    free(ctx);
}

/* ── Encoding ─────────────────────────────────────────────────────── */

int av1_encoder_encode(av1_encoder_ctx_t *ctx, const uint8_t *yuv420, size_t yuv_size, uint8_t *out,
                       size_t *out_size, bool *is_keyframe) {
    if (!ctx || !yuv420 || !out || !out_size)
        return -1;

    bool kf = ctx->need_kf;
    ctx->need_kf = false;

    int width = ctx->config.width;
    int height = ctx->config.height;
    size_t expected = (size_t)(width * height) * 3 / 2;
    if (yuv_size < expected)
        return -1; /* truncated input frame */

    switch (ctx->backend) {
#ifdef HAVE_LIBAOM
        case AV1_BACKEND_LIBAOM: {
            /* Copy YUV planes into libaom image descriptor */
            memcpy(ctx->aom_img.planes[0], yuv420, (size_t)(width * height));
            memcpy(ctx->aom_img.planes[1], yuv420 + width * height, (size_t)(width * height / 4));
            memcpy(ctx->aom_img.planes[2], yuv420 + width * height * 5 / 4,
                   (size_t)(width * height / 4));

            aom_enc_frame_flags_t flags = kf ? AOM_EFLAG_FORCE_KF : 0;
            aom_codec_err_t err =
                aom_codec_encode(ctx->aom_ctx, &ctx->aom_img, (aom_codec_pts_t)ctx->frames_encoded,
                                 1 /* duration */, flags);
            if (err != AOM_CODEC_OK)
                return -1;

            /* Collect all OBU packets from this frame */
            size_t written = 0;
            const aom_codec_cx_pkt_t *pkt;
            aom_codec_iter_t iter = NULL;
            while ((pkt = aom_codec_get_cx_data(ctx->aom_ctx, &iter)) != NULL) {
                if (pkt->kind == AOM_CODEC_CX_FRAME_PKT) {
                    if (written + pkt->data.frame.sz > *out_size)
                        return -1;
                    memcpy(out + written, pkt->data.frame.buf, pkt->data.frame.sz);
                    written += pkt->data.frame.sz;
                    if (pkt->data.frame.flags & AOM_FRAME_IS_KEY)
                        kf = true;
                }
            }
            *out_size = written;
            if (is_keyframe)
                *is_keyframe = kf;
            ctx->frames_encoded++;
            ctx->bytes_out_total += written;
            return 0;
        }
#endif /* HAVE_LIBAOM */

#ifdef HAVE_SVT_AV1
        case AV1_BACKEND_SVT: {
            /* Set up input buffer header for one YUV420 frame */
            EbSvtIOFormat *svt_pic = (EbSvtIOFormat *)ctx->svt_in->p_buffer;
            if (!svt_pic) {
                svt_pic = calloc(1, sizeof(EbSvtIOFormat));
                if (!svt_pic)
                    return -1;
                ctx->svt_in->p_buffer = (uint8_t *)svt_pic;
            }
            /* Point planes at the caller's yuv420 buffer (zero-copy) */
            svt_pic->luma = (uint8_t *)yuv420;
            svt_pic->cb = (uint8_t *)yuv420 + width * height;
            svt_pic->cr = (uint8_t *)yuv420 + width * height * 5 / 4;
            svt_pic->y_stride = (uint32_t)width;
            svt_pic->cb_stride = svt_pic->cr_stride = (uint32_t)(width / 2);
            svt_pic->color_fmt = SVT_AV1_420;

            ctx->svt_in->flags = 0;
            ctx->svt_in->pic_type = kf ? EB_AV1_KEY_PICTURE : EB_AV1_INTER_PICTURE;
            ctx->svt_in->pts = (int64_t)ctx->frames_encoded;
            ctx->svt_in->n_filled_len = (uint32_t)(width * height * 3 / 2);
            ctx->svt_in->p_app_private = NULL;

            if (svt_av1_enc_send_picture(ctx->svt_handle, ctx->svt_in) != EB_ErrorNone)
                return -1;

            /* Retrieve compressed output */
            EbBufferHeaderType *out_buf = NULL;
            if (svt_av1_enc_get_packet(ctx->svt_handle, &out_buf, 0) != EB_ErrorNone)
                return -1;

            if (!out_buf || out_buf->n_filled_len > *out_size) {
                if (out_buf)
                    svt_av1_enc_release_out_buffer(&out_buf);
                return -1;
            }
            memcpy(out, out_buf->p_buffer, out_buf->n_filled_len);
            *out_size = out_buf->n_filled_len;
            bool is_kf = (out_buf->pic_type == EB_AV1_KEY_PICTURE);
            svt_av1_enc_release_out_buffer(&out_buf);

            if (is_keyframe)
                *is_keyframe = is_kf;
            ctx->frames_encoded++;
            ctx->bytes_out_total += *out_size;
            return 0;
        }
#endif /* HAVE_SVT_AV1 */

        default:
            return -1;
    }
}

void av1_encoder_request_keyframe(av1_encoder_ctx_t *ctx) {
    if (ctx)
        ctx->need_kf = true;
}

av1_backend_t av1_encoder_get_backend(const av1_encoder_ctx_t *ctx) {
    return ctx ? ctx->backend : AV1_BACKEND_NONE;
}

void av1_encoder_get_stats(const av1_encoder_ctx_t *ctx, uint32_t *bitrate_kbps,
                           float *fps_actual) {
    if (!ctx)
        return;
    /* Rough estimate: use total bytes and total frames.
     * Production code would use a sliding window over the last second. */
    if (bitrate_kbps && ctx->frames_encoded > 0)
        *bitrate_kbps =
            (uint32_t)(ctx->bytes_out_total * 8 / ctx->frames_encoded / 1000 * ctx->config.fps);
    if (fps_actual)
        *fps_actual = (float)ctx->config.fps;
}
