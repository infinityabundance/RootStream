#ifndef RECORDING_PRESETS_H
#define RECORDING_PRESETS_H

#include "recording_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RecordingPresetConfig {
    enum VideoCodec video_codec;
    const char *h264_preset;       // libx264 preset
    uint32_t h264_bitrate_kbps;
    int h264_crf;                  // Constant Rate Factor (0-51, lower=better)
    int vp9_cpu_used;              // 0-8 (lower=better quality, slower)
    uint32_t vp9_bitrate_kbps;
    int av1_cpu_used;              // 0-8
    uint32_t av1_bitrate_kbps;
    enum AudioCodec audio_codec;
    enum ContainerFormat container;
    const char *description;
};

// Preset definitions
static const struct RecordingPresetConfig RECORDING_PRESETS[] = {
    // PRESET_FAST: H.264 "veryfast", 20Mbps, AAC, MP4
    {
        .video_codec = VIDEO_CODEC_H264,
        .h264_preset = "veryfast",
        .h264_bitrate_kbps = 20000,
        .h264_crf = 23,
        .audio_codec = AUDIO_CODEC_AAC,
        .container = CONTAINER_MP4,
        .description = "Fast encoding - H.264 veryfast preset, 20Mbps, AAC, MP4"
    },
    
    // PRESET_BALANCED: H.264 "medium", 8Mbps, Opus pass, MP4
    {
        .video_codec = VIDEO_CODEC_H264,
        .h264_preset = "medium",
        .h264_bitrate_kbps = 8000,
        .h264_crf = 23,
        .audio_codec = AUDIO_CODEC_OPUS,
        .container = CONTAINER_MP4,
        .description = "Balanced - H.264 medium preset, 8Mbps, Opus, MP4"
    },
    
    // PRESET_HIGH_QUALITY: VP9 cpu_used=2, 5Mbps, Opus pass, MKV
    {
        .video_codec = VIDEO_CODEC_VP9,
        .vp9_cpu_used = 2,
        .vp9_bitrate_kbps = 5000,
        .audio_codec = AUDIO_CODEC_OPUS,
        .container = CONTAINER_MATROSKA,
        .description = "High Quality - VP9 cpu_used=2, 5Mbps, Opus, MKV"
    },
    
    // PRESET_ARCHIVAL: AV1 cpu_used=4, 2Mbps, Opus pass, MKV
    {
        .video_codec = VIDEO_CODEC_AV1,
        .av1_cpu_used = 4,
        .av1_bitrate_kbps = 2000,
        .audio_codec = AUDIO_CODEC_OPUS,
        .container = CONTAINER_MATROSKA,
        .description = "Archival - AV1 cpu_used=4, 2Mbps, Opus, MKV (slow encoding)"
    }
};

// Get preset configuration
static inline const struct RecordingPresetConfig* get_recording_preset(enum RecordingPreset preset) {
    if (preset >= sizeof(RECORDING_PRESETS) / sizeof(RECORDING_PRESETS[0])) {
        return &RECORDING_PRESETS[PRESET_BALANCED];  // Default
    }
    return &RECORDING_PRESETS[preset];
}

#ifdef __cplusplus
}
#endif

#endif /* RECORDING_PRESETS_H */
