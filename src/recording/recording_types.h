#ifndef RECORDING_TYPES_H
#define RECORDING_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_RECORDING_QUEUE_SIZE 512
#define MAX_RECORDINGS 100
#define DEFAULT_REPLAY_BUFFER_SIZE_MB 500

enum VideoCodec {
    VIDEO_CODEC_H264,      // Primary (fast, universal)
    VIDEO_CODEC_VP9,       // Open-source (better compression)
    VIDEO_CODEC_AV1,       // Future (best compression)
};

enum AudioCodec {
    AUDIO_CODEC_OPUS,      // Passthrough (no re-encode)
    AUDIO_CODEC_AAC,       // Fallback (compatible)
};

enum RecordingPreset {
    PRESET_FAST,           // H.264, 1-pass, ~20Mbps
    PRESET_BALANCED,       // H.264, 2-pass, ~8-10Mbps
    PRESET_HIGH_QUALITY,   // VP9, ~5-8Mbps
    PRESET_ARCHIVAL,       // AV1, ~2-4Mbps
};

enum ContainerFormat {
    CONTAINER_MP4,         // Universal (H.264/AAC)
    CONTAINER_MATROSKA,    // Advanced (any codec combo)
};

typedef struct {
    uint32_t recording_id;
    char filename[512];
    char filepath[1024];
    uint64_t creation_time_us;
    uint64_t start_time_us;
    uint64_t duration_us;
    uint64_t file_size_bytes;
    
    enum VideoCodec video_codec;
    enum AudioCodec audio_codec;
    enum ContainerFormat container;
    enum RecordingPreset preset;
    
    uint32_t video_width;
    uint32_t video_height;
    uint32_t video_fps;
    uint32_t video_bitrate_kbps;
    
    uint32_t audio_sample_rate;
    uint8_t audio_channels;
    uint32_t audio_bitrate_kbps;
    
    bool is_complete;
    bool is_paused;
    
    char metadata[512];    // Game name, etc
} recording_info_t;

typedef struct {
    uint8_t *data;
    size_t size;
    uint64_t timestamp_us;
    uint32_t frame_number;
} video_frame_t;

typedef struct {
    float *samples;
    size_t sample_count;
    uint64_t timestamp_us;
} audio_chunk_t;

#ifdef __cplusplus
}
#endif

#endif /* RECORDING_TYPES_H */
