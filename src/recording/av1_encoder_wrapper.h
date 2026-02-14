#ifndef AV1_ENCODER_WRAPPER_H
#define AV1_ENCODER_WRAPPER_H

#include "recording_types.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for FFmpeg types
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

typedef struct {
    struct AVCodecContext *codec_ctx;
    struct AVFrame *frame;
    struct AVPacket *packet;
    struct SwsContext *sws_ctx;
    
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    uint32_t bitrate_kbps;
    
    int cpu_used;        // AV1 speed parameter (0-8, higher = faster but lower quality)
    int crf;             // Constant Rate Factor (0-63, lower = better quality)
    
    uint64_t frame_count;
    bool initialized;
} av1_encoder_t;

/**
 * Initialize AV1 encoder
 * 
 * @param encoder     Encoder context to initialize
 * @param width       Video width in pixels
 * @param height      Video height in pixels
 * @param fps         Target framerate
 * @param bitrate_kbps Target bitrate in kbps (0 = use CRF mode)
 * @param cpu_used    CPU usage parameter (0-8, higher = faster, lower quality)
 * @param crf         Constant Rate Factor (0-63, lower = better, -1 = use bitrate mode)
 * @return            0 on success, -1 on error
 */
int av1_encoder_init(av1_encoder_t *encoder,
                     uint32_t width, uint32_t height,
                     uint32_t fps, uint32_t bitrate_kbps,
                     int cpu_used, int crf);

/**
 * Encode a single frame
 * 
 * @param encoder       Encoder context
 * @param frame_data    Input frame data (RGB, RGBA, or YUV format)
 * @param pixel_format  Pixel format string ("rgb", "rgba", "yuv420p", etc.)
 * @param output        Output buffer for encoded data
 * @param output_size   Size of encoded data (output parameter)
 * @param is_keyframe   Whether encoded frame is a keyframe (output parameter)
 * @return              0 on success, -1 on error
 */
int av1_encoder_encode_frame(av1_encoder_t *encoder,
                             const uint8_t *frame_data,
                             const char *pixel_format,
                             uint8_t **output,
                             size_t *output_size,
                             bool *is_keyframe);

/**
 * Request next frame to be a keyframe
 * 
 * @param encoder  Encoder context
 * @return         0 on success, -1 on error
 */
int av1_encoder_request_keyframe(av1_encoder_t *encoder);

/**
 * Update encoder bitrate dynamically
 * 
 * @param encoder       Encoder context
 * @param bitrate_kbps  New target bitrate in kbps
 * @return              0 on success, -1 on error
 */
int av1_encoder_set_bitrate(av1_encoder_t *encoder, uint32_t bitrate_kbps);

/**
 * Get encoder statistics
 * 
 * @param encoder      Encoder context
 * @param frames_out   Number of frames encoded (output parameter)
 * @return             0 on success, -1 on error
 */
int av1_encoder_get_stats(av1_encoder_t *encoder, uint64_t *frames_out);

/**
 * Flush encoder and get any remaining packets
 * 
 * @param encoder  Encoder context
 * @return         0 on success, -1 on error
 */
int av1_encoder_flush(av1_encoder_t *encoder);

/**
 * Cleanup and free encoder resources
 * 
 * @param encoder  Encoder context to cleanup
 */
void av1_encoder_cleanup(av1_encoder_t *encoder);

/**
 * Check if AV1 encoder is available on this system
 * 
 * @return  true if available, false otherwise
 */
bool av1_encoder_available(void);

#ifdef __cplusplus
}
#endif

#endif /* AV1_ENCODER_WRAPPER_H */
