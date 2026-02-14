#include "vp9_encoder_wrapper.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

// Helper function to convert pixel format string to AVPixelFormat
static enum AVPixelFormat string_to_av_pixfmt(const char *pixel_format) {
    if (strcmp(pixel_format, "rgb") == 0 || strcmp(pixel_format, "rgb24") == 0) {
        return AV_PIX_FMT_RGB24;
    } else if (strcmp(pixel_format, "rgba") == 0 || strcmp(pixel_format, "rgba32") == 0) {
        return AV_PIX_FMT_RGBA;
    } else if (strcmp(pixel_format, "bgr") == 0 || strcmp(pixel_format, "bgr24") == 0) {
        return AV_PIX_FMT_BGR24;
    } else if (strcmp(pixel_format, "bgra") == 0) {
        return AV_PIX_FMT_BGRA;
    } else if (strcmp(pixel_format, "yuv420p") == 0) {
        return AV_PIX_FMT_YUV420P;
    }
    return AV_PIX_FMT_NONE;
}

bool vp9_encoder_available(void) {
    const AVCodec *codec = avcodec_find_encoder_by_name("libvpx-vp9");
    return (codec != nullptr);
}

int vp9_encoder_init(vp9_encoder_t *encoder,
                     uint32_t width, uint32_t height,
                     uint32_t fps, uint32_t bitrate_kbps,
                     int cpu_used, int quality) {
    if (!encoder) {
        fprintf(stderr, "VP9 Encoder: NULL encoder context\n");
        return -1;
    }
    
    memset(encoder, 0, sizeof(vp9_encoder_t));
    
    // Find VP9 encoder
    const AVCodec *codec = avcodec_find_encoder_by_name("libvpx-vp9");
    if (!codec) {
        fprintf(stderr, "VP9 Encoder: libvpx-vp9 codec not found\n");
        return -1;
    }
    
    // Allocate codec context
    encoder->codec_ctx = avcodec_alloc_context3(codec);
    if (!encoder->codec_ctx) {
        fprintf(stderr, "VP9 Encoder: Failed to allocate codec context\n");
        return -1;
    }
    
    // Set basic parameters
    encoder->codec_ctx->width = width;
    encoder->codec_ctx->height = height;
    encoder->codec_ctx->time_base = (AVRational){1, (int)fps};
    encoder->codec_ctx->framerate = (AVRational){(int)fps, 1};
    encoder->codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    encoder->codec_ctx->gop_size = fps * 2;  // Keyframe every 2 seconds
    encoder->codec_ctx->max_b_frames = 0;     // VP9 typically doesn't use B-frames
    
    // Set bitrate or quality mode
    if (quality >= 0 && quality <= 63) {
        // CQ (Constrained Quality) mode
        encoder->codec_ctx->flags |= AV_CODEC_FLAG_QSCALE;
        encoder->codec_ctx->global_quality = quality;
        av_opt_set(encoder->codec_ctx->priv_data, "crf", std::to_string(quality).c_str(), 0);
        av_opt_set(encoder->codec_ctx->priv_data, "b:v", "0", 0);
    } else {
        // Bitrate mode
        encoder->codec_ctx->bit_rate = bitrate_kbps * 1000;
        encoder->codec_ctx->rc_max_rate = bitrate_kbps * 1000;
        encoder->codec_ctx->rc_min_rate = bitrate_kbps * 1000;
        encoder->codec_ctx->rc_buffer_size = bitrate_kbps * 1000 * 2;
    }
    
    // Set VP9-specific options
    // cpu-used: 0 = slowest/best quality, 5 = fastest/lower quality
    int cpu_used_value = (cpu_used >= 0 && cpu_used <= 5) ? cpu_used : 2;
    av_opt_set(encoder->codec_ctx->priv_data, "cpu-used", std::to_string(cpu_used_value).c_str(), 0);
    
    // Set deadline (good quality mode)
    av_opt_set(encoder->codec_ctx->priv_data, "deadline", "good", 0);
    
    // Enable row-based multithreading for better performance
    av_opt_set(encoder->codec_ctx->priv_data, "row-mt", "1", 0);
    
    // Set tile columns for parallel encoding (auto-select based on width)
    if (width >= 1920) {
        av_opt_set(encoder->codec_ctx->priv_data, "tile-columns", "2", 0);
    } else if (width >= 1280) {
        av_opt_set(encoder->codec_ctx->priv_data, "tile-columns", "1", 0);
    }
    
    // Open codec
    int ret = avcodec_open2(encoder->codec_ctx, codec, nullptr);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "VP9 Encoder: Failed to open codec: %s\n", errbuf);
        avcodec_free_context(&encoder->codec_ctx);
        return -1;
    }
    
    // Allocate frame
    encoder->frame = av_frame_alloc();
    if (!encoder->frame) {
        fprintf(stderr, "VP9 Encoder: Failed to allocate frame\n");
        avcodec_free_context(&encoder->codec_ctx);
        return -1;
    }
    
    encoder->frame->format = encoder->codec_ctx->pix_fmt;
    encoder->frame->width = width;
    encoder->frame->height = height;
    
    ret = av_frame_get_buffer(encoder->frame, 0);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "VP9 Encoder: Failed to allocate frame buffer: %s\n", errbuf);
        av_frame_free(&encoder->frame);
        avcodec_free_context(&encoder->codec_ctx);
        return -1;
    }
    
    // Allocate packet
    encoder->packet = av_packet_alloc();
    if (!encoder->packet) {
        fprintf(stderr, "VP9 Encoder: Failed to allocate packet\n");
        av_frame_free(&encoder->frame);
        avcodec_free_context(&encoder->codec_ctx);
        return -1;
    }
    
    // Store parameters
    encoder->width = width;
    encoder->height = height;
    encoder->fps = fps;
    encoder->bitrate_kbps = bitrate_kbps;
    encoder->cpu_used = cpu_used_value;
    encoder->quality = quality;
    encoder->frame_count = 0;
    encoder->initialized = true;
    encoder->sws_ctx = nullptr;  // Will be initialized on first frame
    
    printf("VP9 Encoder initialized: %ux%u @ %u fps, cpu-used=%d\n",
           width, height, fps, cpu_used_value);
    
    return 0;
}

int vp9_encoder_encode_frame(vp9_encoder_t *encoder,
                             const uint8_t *frame_data,
                             const char *pixel_format,
                             uint8_t **output,
                             size_t *output_size,
                             bool *is_keyframe) {
    if (!encoder || !encoder->initialized || !frame_data || !pixel_format) {
        fprintf(stderr, "VP9 Encoder: Invalid parameters\n");
        return -1;
    }
    
    // Convert input format to AVPixelFormat
    enum AVPixelFormat src_fmt = string_to_av_pixfmt(pixel_format);
    if (src_fmt == AV_PIX_FMT_NONE) {
        fprintf(stderr, "VP9 Encoder: Unsupported pixel format: %s\n", pixel_format);
        return -1;
    }
    
    // Initialize swscale context if needed
    if (!encoder->sws_ctx) {
        encoder->sws_ctx = sws_getContext(
            encoder->width, encoder->height, src_fmt,
            encoder->width, encoder->height, AV_PIX_FMT_YUV420P,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        
        if (!encoder->sws_ctx) {
            fprintf(stderr, "VP9 Encoder: Failed to initialize swscale context\n");
            return -1;
        }
    }
    
    // Convert frame to YUV420P
    int src_linesize[4] = {0};
    av_image_fill_linesizes(src_linesize, src_fmt, encoder->width);
    
    const uint8_t *src_data[4] = {frame_data, nullptr, nullptr, nullptr};
    
    int ret = sws_scale(encoder->sws_ctx,
                       src_data, src_linesize, 0, encoder->height,
                       encoder->frame->data, encoder->frame->linesize);
    if (ret < 0) {
        fprintf(stderr, "VP9 Encoder: sws_scale failed\n");
        return -1;
    }
    
    // Set frame PTS
    encoder->frame->pts = encoder->frame_count;
    
    // Send frame to encoder
    ret = avcodec_send_frame(encoder->codec_ctx, encoder->frame);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "VP9 Encoder: Failed to send frame: %s\n", errbuf);
        return -1;
    }
    
    // Receive encoded packet
    ret = avcodec_receive_packet(encoder->codec_ctx, encoder->packet);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        // No packet available yet
        *output_size = 0;
        encoder->frame_count++;
        return 0;
    } else if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "VP9 Encoder: Failed to receive packet: %s\n", errbuf);
        return -1;
    }
    
    // Copy encoded data
    *output = (uint8_t *)malloc(encoder->packet->size);
    if (!*output) {
        fprintf(stderr, "VP9 Encoder: Failed to allocate output buffer\n");
        av_packet_unref(encoder->packet);
        return -1;
    }
    
    memcpy(*output, encoder->packet->data, encoder->packet->size);
    *output_size = encoder->packet->size;
    
    // Check if keyframe
    if (is_keyframe) {
        *is_keyframe = (encoder->packet->flags & AV_PKT_FLAG_KEY) != 0;
    }
    
    av_packet_unref(encoder->packet);
    encoder->frame_count++;
    
    return 0;
}

int vp9_encoder_request_keyframe(vp9_encoder_t *encoder) {
    if (!encoder || !encoder->initialized) {
        return -1;
    }
    
    encoder->frame->pict_type = AV_PICTURE_TYPE_I;
    encoder->frame->key_frame = 1;
    
    return 0;
}

int vp9_encoder_set_bitrate(vp9_encoder_t *encoder, uint32_t bitrate_kbps) {
    if (!encoder || !encoder->initialized) {
        return -1;
    }
    
    encoder->codec_ctx->bit_rate = bitrate_kbps * 1000;
    encoder->codec_ctx->rc_max_rate = bitrate_kbps * 1000;
    encoder->codec_ctx->rc_min_rate = bitrate_kbps * 1000;
    encoder->bitrate_kbps = bitrate_kbps;
    
    return 0;
}

int vp9_encoder_get_stats(vp9_encoder_t *encoder, uint64_t *frames_out) {
    if (!encoder || !encoder->initialized) {
        return -1;
    }
    
    if (frames_out) {
        *frames_out = encoder->frame_count;
    }
    
    return 0;
}

int vp9_encoder_flush(vp9_encoder_t *encoder) {
    if (!encoder || !encoder->initialized) {
        return -1;
    }
    
    // Send NULL frame to flush encoder
    int ret = avcodec_send_frame(encoder->codec_ctx, nullptr);
    if (ret < 0) {
        return -1;
    }
    
    // Receive all remaining packets
    while (true) {
        ret = avcodec_receive_packet(encoder->codec_ctx, encoder->packet);
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
            break;
        }
        av_packet_unref(encoder->packet);
    }
    
    return 0;
}

void vp9_encoder_cleanup(vp9_encoder_t *encoder) {
    if (!encoder) {
        return;
    }
    
    if (encoder->sws_ctx) {
        sws_freeContext(encoder->sws_ctx);
        encoder->sws_ctx = nullptr;
    }
    
    if (encoder->packet) {
        av_packet_free(&encoder->packet);
    }
    
    if (encoder->frame) {
        av_frame_free(&encoder->frame);
    }
    
    if (encoder->codec_ctx) {
        avcodec_free_context(&encoder->codec_ctx);
    }
    
    encoder->initialized = false;
}
