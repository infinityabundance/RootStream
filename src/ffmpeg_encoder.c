/*
 * ffmpeg_encoder.c - Software H.264/H.265 encoding via FFmpeg/libx264
 *
 * Pure CPU-based encoding fallback for systems without GPU hardware encoding.
 * ~10-20x slower than hardware encoding, but works everywhere.
 * 
 * Requires: libavcodec, libavutil, libswscale
 * Codecs: libx264 (H.264), libx265 (H.265)
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef HAVE_FFMPEG

#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

typedef struct {
    AVCodecContext *codec_ctx;
    AVFrame *frame;
    AVPacket *packet;
    struct SwsContext *sws_ctx;
    int width;
    int height;
    int fps;
    codec_type_t codec;
    uint64_t frame_count;
} ffmpeg_ctx_t;

/*
 * Check if FFmpeg software encoding is available
 */
bool rootstream_encoder_ffmpeg_available(void) {
    /* Check for libx264 codec */
    const AVCodec *codec = avcodec_find_encoder_by_name("libx264");
    if (codec) {
        return true;
    }
    
    /* Also check for h264_nvenc as fallback (though we prefer direct NVENC) */
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    return (codec != NULL);
}

/*
 * Detect if H.264 NAL stream contains an IDR (keyframe)
 */
static bool detect_h264_keyframe_ffmpeg(const uint8_t *data, size_t size) {
    if (!data || size < 5) {
        return false;
    }

    for (size_t i = 0; i < size - 4; i++) {
        bool sc3 = (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x01);
        bool sc4 = (i + 4 < size && data[i] == 0x00 && data[i+1] == 0x00 &&
                   data[i+2] == 0x00 && data[i+3] == 0x01);

        if (sc3 || sc4) {
            size_t idx = sc4 ? i + 4 : i + 3;
            if (idx < size && (data[idx] & 0x1F) == 5) {
                return true;  /* IDR slice */
            }
            i += sc4 ? 3 : 2;
        }
    }
    return false;
}

/*
 * Initialize FFmpeg software encoder
 */
int rootstream_encoder_init_ffmpeg(rootstream_ctx_t *ctx, codec_type_t codec) {
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context\n");
        return -1;
    }

    /* Allocate FFmpeg context */
    ffmpeg_ctx_t *ff = calloc(1, sizeof(ffmpeg_ctx_t));
    if (!ff) {
        fprintf(stderr, "ERROR: Cannot allocate FFmpeg context\n");
        return -1;
    }

    ff->codec = codec;
    ff->width = ctx->display.width;
    ff->height = ctx->display.height;
    ff->fps = ctx->display.refresh_rate ? ctx->display.refresh_rate : 60;
    ff->frame_count = 0;

    /* Find encoder codec */
    const AVCodec *avcodec = NULL;
    const char *codec_name = NULL;

    if (codec == CODEC_H265) {
        avcodec = avcodec_find_encoder_by_name("libx265");
        codec_name = "H.265/HEVC";
    } else {
        avcodec = avcodec_find_encoder_by_name("libx264");
        codec_name = "H.264/AVC";
    }

    if (!avcodec) {
        fprintf(stderr, "ERROR: FFmpeg encoder not found for %s\n", codec_name);
        free(ff);
        return -1;
    }

    /* Allocate codec context */
    ff->codec_ctx = avcodec_alloc_context3(avcodec);
    if (!ff->codec_ctx) {
        fprintf(stderr, "ERROR: Cannot allocate codec context\n");
        free(ff);
        return -1;
    }

    /* Set encoding parameters */
    ff->codec_ctx->width = ff->width;
    ff->codec_ctx->height = ff->height;
    ff->codec_ctx->time_base = (AVRational){1, ff->fps};
    ff->codec_ctx->framerate = (AVRational){ff->fps, 1};
    ff->codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    
    /* Bitrate */
    if (ctx->encoder.bitrate > 0) {
        ff->codec_ctx->bit_rate = ctx->encoder.bitrate;
    } else {
        ff->codec_ctx->bit_rate = 5000000;  /* 5 Mbps default */
    }

    /* GOP size (keyframe interval) */
    ff->codec_ctx->gop_size = ff->fps * 2;  /* Keyframe every 2 seconds */
    ff->codec_ctx->max_b_frames = 0;  /* No B-frames for low latency */

    /* x264 specific settings for low latency */
    if (codec == CODEC_H264) {
        /* Use "faster" preset for better performance */
        av_opt_set(ff->codec_ctx->priv_data, "preset", "faster", 0);
        /* Tune for zero-latency streaming */
        av_opt_set(ff->codec_ctx->priv_data, "tune", "zerolatency", 0);
        /* Disable B-frames explicitly */
        av_opt_set(ff->codec_ctx->priv_data, "bframes", "0", 0);
    } else if (codec == CODEC_H265) {
        /* x265 settings for low latency */
        av_opt_set(ff->codec_ctx->priv_data, "preset", "fast", 0);
        av_opt_set(ff->codec_ctx->priv_data, "tune", "zerolatency", 0);
    }

    /* Open codec */
    int ret = avcodec_open2(ff->codec_ctx, avcodec, NULL);
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "ERROR: Cannot open %s codec: %s\n", codec_name, errbuf);
        avcodec_free_context(&ff->codec_ctx);
        free(ff);
        return -1;
    }

    /* Allocate frame */
    ff->frame = av_frame_alloc();
    if (!ff->frame) {
        fprintf(stderr, "ERROR: Cannot allocate frame\n");
        avcodec_free_context(&ff->codec_ctx);
        free(ff);
        return -1;
    }

    ff->frame->format = ff->codec_ctx->pix_fmt;
    ff->frame->width = ff->width;
    ff->frame->height = ff->height;

    ret = av_frame_get_buffer(ff->frame, 0);
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "ERROR: Cannot allocate frame buffer: %s\n", errbuf);
        av_frame_free(&ff->frame);
        avcodec_free_context(&ff->codec_ctx);
        free(ff);
        return -1;
    }

    /* Allocate packet */
    ff->packet = av_packet_alloc();
    if (!ff->packet) {
        fprintf(stderr, "ERROR: Cannot allocate packet\n");
        av_frame_free(&ff->frame);
        avcodec_free_context(&ff->codec_ctx);
        free(ff);
        return -1;
    }

    /* Initialize swscale context for RGBA to YUV420P conversion */
    ff->sws_ctx = sws_getContext(
        ff->width, ff->height, AV_PIX_FMT_RGBA,
        ff->width, ff->height, AV_PIX_FMT_YUV420P,
        SWS_FAST_BILINEAR, NULL, NULL, NULL
    );

    if (!ff->sws_ctx) {
        fprintf(stderr, "ERROR: Cannot initialize swscale context\n");
        av_packet_free(&ff->packet);
        av_frame_free(&ff->frame);
        avcodec_free_context(&ff->codec_ctx);
        free(ff);
        return -1;
    }

    /* Store in encoder context */
    ctx->encoder.type = ENCODER_FFMPEG;
    ctx->encoder.codec = codec;
    ctx->encoder.hw_ctx = ff;
    ctx->encoder.bitrate = ff->codec_ctx->bit_rate;
    ctx->encoder.framerate = ff->fps;
    ctx->encoder.low_latency = true;
    ctx->encoder.max_output_size = (size_t)ff->width * ff->height * 4;

    printf("✓ FFmpeg %s encoder ready: %dx%d @ %d fps, %d kbps (software)\n",
           codec_name, ff->width, ff->height, ff->fps, 
           ctx->encoder.bitrate / 1000);
    printf("  ⚠ WARNING: Using CPU encoding - performance may be limited\n");

    return 0;
}

/*
 * Encode a frame with FFmpeg
 */
int rootstream_encode_frame_ffmpeg(rootstream_ctx_t *ctx, frame_buffer_t *in,
                                   uint8_t *out, size_t *out_size) {
    if (!ctx || !in || !out || !out_size) {
        return -1;
    }

    ffmpeg_ctx_t *ff = (ffmpeg_ctx_t*)ctx->encoder.hw_ctx;
    if (!ff) {
        fprintf(stderr, "ERROR: FFmpeg encoder not initialized\n");
        return -1;
    }

    /* Convert RGBA to YUV420P */
    const uint8_t *src_data[1] = { in->data };
    int src_linesize[1] = { (int)in->pitch };

    int ret = sws_scale(
        ff->sws_ctx,
        src_data, src_linesize, 0, ff->height,
        ff->frame->data, ff->frame->linesize
    );

    if (ret < 0) {
        fprintf(stderr, "ERROR: Color conversion failed\n");
        return -1;
    }

    /* Set frame parameters */
    ff->frame->pts = ff->frame_count++;

    /* Check if we should force keyframe */
    if (ctx->encoder.force_keyframe) {
        ff->frame->pict_type = AV_PICTURE_TYPE_I;
        ctx->encoder.force_keyframe = false;
    } else {
        ff->frame->pict_type = AV_PICTURE_TYPE_NONE;
    }

    /* Send frame to encoder */
    ret = avcodec_send_frame(ff->codec_ctx, ff->frame);
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "ERROR: Failed to send frame to encoder: %s\n", errbuf);
        return -1;
    }

    /* Receive encoded packet */
    ret = avcodec_receive_packet(ff->codec_ctx, ff->packet);
    if (ret == AVERROR(EAGAIN)) {
        /* Need more frames */
        *out_size = 0;
        return 0;
    } else if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "ERROR: Failed to receive packet from encoder: %s\n", errbuf);
        return -1;
    }

    /* Copy encoded data */
    if (ff->packet->size > 0) {
        *out_size = ff->packet->size;
        memcpy(out, ff->packet->data, ff->packet->size);

        /* Detect keyframe */
        in->is_keyframe = (ff->packet->flags & AV_PKT_FLAG_KEY) != 0;
    } else {
        *out_size = 0;
    }

    av_packet_unref(ff->packet);
    return 0;
}

/*
 * Encode frame with keyframe detection
 */
int rootstream_encode_frame_ex_ffmpeg(rootstream_ctx_t *ctx, frame_buffer_t *in,
                                      uint8_t *out, size_t *out_size, bool *is_keyframe) {
    int result = rootstream_encode_frame_ffmpeg(ctx, in, out, out_size);
    if (result == 0 && is_keyframe && *out_size > 0) {
        /* Detect keyframe from NAL units */
        *is_keyframe = detect_h264_keyframe_ffmpeg(out, *out_size);
    }
    return result;
}

/*
 * Cleanup FFmpeg encoder
 */
void rootstream_encoder_cleanup_ffmpeg(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->encoder.hw_ctx) {
        return;
    }

    ffmpeg_ctx_t *ff = (ffmpeg_ctx_t*)ctx->encoder.hw_ctx;

    if (ff->sws_ctx) {
        sws_freeContext(ff->sws_ctx);
    }

    if (ff->packet) {
        av_packet_free(&ff->packet);
    }

    if (ff->frame) {
        av_frame_free(&ff->frame);
    }

    if (ff->codec_ctx) {
        avcodec_free_context(&ff->codec_ctx);
    }

    free(ff);
    ctx->encoder.hw_ctx = NULL;
}

#else  /* !HAVE_FFMPEG */

/* Stub implementations when FFmpeg is not available */

bool rootstream_encoder_ffmpeg_available(void) {
    return false;
}

int rootstream_encoder_init_ffmpeg(rootstream_ctx_t *ctx, codec_type_t codec) {
    (void)ctx;
    (void)codec;
    fprintf(stderr, "ERROR: FFmpeg encoder not available (libavcodec not found at build time)\n");
    fprintf(stderr, "FIX: Install libavcodec/libx264 development packages and rebuild\n");
    return -1;
}

int rootstream_encode_frame_ffmpeg(rootstream_ctx_t *ctx, frame_buffer_t *in,
                                   uint8_t *out, size_t *out_size) {
    (void)ctx;
    (void)in;
    (void)out;
    (void)out_size;
    return -1;
}

int rootstream_encode_frame_ex_ffmpeg(rootstream_ctx_t *ctx, frame_buffer_t *in,
                                      uint8_t *out, size_t *out_size, bool *is_keyframe) {
    (void)ctx;
    (void)in;
    (void)out;
    (void)out_size;
    (void)is_keyframe;
    return -1;
}

void rootstream_encoder_cleanup_ffmpeg(rootstream_ctx_t *ctx) {
    (void)ctx;
}

#endif  /* HAVE_FFMPEG */
