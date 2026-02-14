#include "h264_encoder_wrapper.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

/**
 * Check if H.264 encoder is available
 */
bool h264_encoder_available(void) {
    const AVCodec *codec = avcodec_find_encoder_by_name("libx264");
    if (codec) {
        return true;
    }
    
    // Fallback: check for any H.264 encoder
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    return (codec != NULL);
}

/**
 * Initialize H.264 encoder with specified parameters
 */
int h264_encoder_init(h264_encoder_t *encoder,
                     uint32_t width, uint32_t height,
                     uint32_t fps, uint32_t bitrate_kbps,
                     const char *preset, int crf) {
    if (!encoder) {
        fprintf(stderr, "ERROR: NULL encoder context\n");
        return -1;
    }
    
    memset(encoder, 0, sizeof(h264_encoder_t));
    
    encoder->width = width;
    encoder->height = height;
    encoder->fps = fps > 0 ? fps : 60;
    encoder->bitrate_kbps = bitrate_kbps;
    encoder->preset = preset ? preset : "medium";
    encoder->crf = crf;
    encoder->frame_count = 0;
    
    // Find libx264 encoder
    const AVCodec *codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        fprintf(stderr, "ERROR: libx264 codec not found\n");
        return -1;
    }
    
    // Allocate codec context
    encoder->codec_ctx = avcodec_alloc_context3(codec);
    if (!encoder->codec_ctx) {
        fprintf(stderr, "ERROR: Failed to allocate codec context\n");
        return -1;
    }
    
    // Set basic parameters
    encoder->codec_ctx->width = width;
    encoder->codec_ctx->height = height;
    encoder->codec_ctx->time_base = (AVRational){1, (int)fps};
    encoder->codec_ctx->framerate = (AVRational){(int)fps, 1};
    encoder->codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    encoder->codec_ctx->gop_size = fps;  // Keyframe every second
    encoder->codec_ctx->max_b_frames = 0;  // No B-frames for lower latency
    
    // Set encoding mode: CRF or bitrate
    if (crf >= 0 && crf <= 51) {
        // CRF mode (constant quality)
        char crf_str[8];
        snprintf(crf_str, sizeof(crf_str), "%d", crf);
        av_opt_set(encoder->codec_ctx->priv_data, "crf", crf_str, 0);
        printf("INFO: H.264 encoder using CRF mode (crf=%d)\n", crf);
    } else {
        // Bitrate mode
        encoder->codec_ctx->bit_rate = bitrate_kbps * 1000;
        encoder->codec_ctx->rc_buffer_size = bitrate_kbps * 1000 * 2;
        encoder->codec_ctx->rc_max_rate = bitrate_kbps * 1000;
        encoder->codec_ctx->rc_min_rate = bitrate_kbps * 1000 / 2;
        printf("INFO: H.264 encoder using bitrate mode (%u kbps)\n", bitrate_kbps);
    }
    
    // Set preset
    av_opt_set(encoder->codec_ctx->priv_data, "preset", preset, 0);
    
    // Tune for low latency (optional but recommended for streaming)
    av_opt_set(encoder->codec_ctx->priv_data, "tune", "zerolatency", 0);
    
    // Open codec
    int ret = avcodec_open2(encoder->codec_ctx, codec, NULL);
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "ERROR: Failed to open codec: %s\n", errbuf);
        avcodec_free_context(&encoder->codec_ctx);
        return -1;
    }
    
    // Allocate frame
    encoder->frame = av_frame_alloc();
    if (!encoder->frame) {
        fprintf(stderr, "ERROR: Failed to allocate frame\n");
        avcodec_free_context(&encoder->codec_ctx);
        return -1;
    }
    
    encoder->frame->format = encoder->codec_ctx->pix_fmt;
    encoder->frame->width = width;
    encoder->frame->height = height;
    
    ret = av_frame_get_buffer(encoder->frame, 0);
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "ERROR: Failed to allocate frame buffer: %s\n", errbuf);
        av_frame_free(&encoder->frame);
        avcodec_free_context(&encoder->codec_ctx);
        return -1;
    }
    
    // Allocate packet
    encoder->packet = av_packet_alloc();
    if (!encoder->packet) {
        fprintf(stderr, "ERROR: Failed to allocate packet\n");
        av_frame_free(&encoder->frame);
        avcodec_free_context(&encoder->codec_ctx);
        return -1;
    }
    
    encoder->initialized = true;
    printf("✓ H.264 encoder initialized (%dx%d @ %u fps, preset=%s)\n",
           width, height, fps, preset);
    
    return 0;
}

/**
 * Detect pixel format from string
 */
static enum AVPixelFormat detect_pixel_format(const char *format_str) {
    if (!format_str) return AV_PIX_FMT_RGB24;
    
    if (strcmp(format_str, "rgb") == 0 || strcmp(format_str, "rgb24") == 0) {
        return AV_PIX_FMT_RGB24;
    } else if (strcmp(format_str, "rgba") == 0 || strcmp(format_str, "rgba32") == 0) {
        return AV_PIX_FMT_RGBA;
    } else if (strcmp(format_str, "bgr") == 0 || strcmp(format_str, "bgr24") == 0) {
        return AV_PIX_FMT_BGR24;
    } else if (strcmp(format_str, "bgra") == 0 || strcmp(format_str, "bgra32") == 0) {
        return AV_PIX_FMT_BGRA;
    } else if (strcmp(format_str, "yuv420p") == 0) {
        return AV_PIX_FMT_YUV420P;
    }
    
    return AV_PIX_FMT_RGB24;  // Default
}

/**
 * Encode a single frame
 */
int h264_encoder_encode_frame(h264_encoder_t *encoder,
                             const uint8_t *frame_data,
                             const char *pixel_format,
                             uint8_t **output,
                             size_t *output_size,
                             bool *is_keyframe) {
    if (!encoder || !encoder->initialized || !frame_data || !output || !output_size) {
        fprintf(stderr, "ERROR: Invalid encoder parameters\n");
        return -1;
    }
    
    // Detect input pixel format
    enum AVPixelFormat input_format = detect_pixel_format(pixel_format);
    
    // For YUV420P input, we don't need conversion
    if (input_format == AV_PIX_FMT_YUV420P) {
        // YUV420P requires special handling with three planes
        // For now, we require conversion through RGB
        fprintf(stderr, "ERROR: Direct YUV420P input not yet supported, use RGB/RGBA\n");
        return -1;
    }
    
    // Initialize swscale context if needed
    if (!encoder->sws_ctx) {
        encoder->sws_ctx = sws_getContext(
            encoder->width, encoder->height, input_format,
            encoder->width, encoder->height, encoder->codec_ctx->pix_fmt,
            SWS_FAST_BILINEAR, NULL, NULL, NULL
        );
        
        if (!encoder->sws_ctx) {
            fprintf(stderr, "ERROR: Failed to create swscale context\n");
            return -1;
        }
    }
    
    // Calculate stride for input format (RGB/RGBA/BGR/BGRA only)
    int bytes_per_pixel = (input_format == AV_PIX_FMT_RGBA || 
                          input_format == AV_PIX_FMT_BGRA) ? 4 : 3;
    int src_stride = encoder->width * bytes_per_pixel;
    
    // Convert to YUV420P
    const uint8_t *src_data[1] = { frame_data };
    int src_stride_arr[1] = { src_stride };
    
    sws_scale(encoder->sws_ctx, src_data, src_stride_arr, 0, encoder->height,
              encoder->frame->data, encoder->frame->linesize);
    
    // Set presentation timestamp
    encoder->frame->pts = encoder->frame_count++;
    
    // Send frame to encoder
    int ret = avcodec_send_frame(encoder->codec_ctx, encoder->frame);
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "ERROR: Failed to send frame: %s\n", errbuf);
        return -1;
    }
    
    // Receive encoded packet
    ret = avcodec_receive_packet(encoder->codec_ctx, encoder->packet);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        // Need more frames or end of stream
        *output_size = 0;
        return 0;
    } else if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "ERROR: Failed to receive packet: %s\n", errbuf);
        return -1;
    }
    
    // Allocate output buffer
    *output = (uint8_t*)malloc(encoder->packet->size);
    if (!*output) {
        fprintf(stderr, "ERROR: Failed to allocate output buffer\n");
        av_packet_unref(encoder->packet);
        return -1;
    }
    
    // Copy encoded data
    memcpy(*output, encoder->packet->data, encoder->packet->size);
    *output_size = encoder->packet->size;
    
    // Check if keyframe
    if (is_keyframe) {
        *is_keyframe = (encoder->packet->flags & AV_PKT_FLAG_KEY) != 0;
    }
    
    av_packet_unref(encoder->packet);
    
    return 0;
}

/**
 * Request next frame to be a keyframe
 */
int h264_encoder_request_keyframe(h264_encoder_t *encoder) {
    if (!encoder || !encoder->initialized) {
        return -1;
    }
    
    // Force keyframe by setting picture type and flags
    encoder->frame->pict_type = AV_PICTURE_TYPE_I;
    encoder->frame->flags |= AV_FRAME_FLAG_KEY;
    
    return 0;
}

/**
 * Update encoder bitrate dynamically
 */
int h264_encoder_set_bitrate(h264_encoder_t *encoder, uint32_t bitrate_kbps) {
    if (!encoder || !encoder->initialized) {
        return -1;
    }
    
    encoder->bitrate_kbps = bitrate_kbps;
    encoder->codec_ctx->bit_rate = bitrate_kbps * 1000;
    encoder->codec_ctx->rc_max_rate = bitrate_kbps * 1000;
    
    printf("INFO: H.264 encoder bitrate updated to %u kbps\n", bitrate_kbps);
    
    return 0;
}

/**
 * Get encoder statistics
 */
int h264_encoder_get_stats(h264_encoder_t *encoder, uint64_t *frames_out) {
    if (!encoder || !encoder->initialized || !frames_out) {
        return -1;
    }
    
    *frames_out = encoder->frame_count;
    return 0;
}

/**
 * Flush encoder
 */
int h264_encoder_flush(h264_encoder_t *encoder) {
    if (!encoder || !encoder->initialized) {
        return -1;
    }
    
    // Send NULL frame to signal end of stream
    avcodec_send_frame(encoder->codec_ctx, NULL);
    
    // Receive remaining packets
    while (1) {
        int ret = avcodec_receive_packet(encoder->codec_ctx, encoder->packet);
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
            break;
        }
        av_packet_unref(encoder->packet);
    }
    
    return 0;
}

/**
 * Cleanup encoder resources
 */
void h264_encoder_cleanup(h264_encoder_t *encoder) {
    if (!encoder) return;
    
    if (encoder->sws_ctx) {
        sws_freeContext(encoder->sws_ctx);
        encoder->sws_ctx = NULL;
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
    printf("✓ H.264 encoder cleaned up\n");
}
