#ifndef RECORDING_MANAGER_H
#define RECORDING_MANAGER_H

#include "recording_types.h"
#include "disk_manager.h"
#include "recording_metadata.h"
#include "replay_buffer.h"
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

// Forward declarations
struct AVFormatContext;
struct AVStream;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;

// Encoder wrappers forward declarations
struct h264_encoder;
struct vp9_encoder;
struct av1_encoder;

class RecordingManager {
private:
    struct {
        char output_directory[1024];
        uint64_t max_storage_mb;
        uint32_t auto_cleanup_threshold_percent;
        bool auto_cleanup_enabled;
    } config;
    
    recording_info_t active_recording;
    recording_metadata_t metadata;
    std::atomic<bool> is_recording;
    std::atomic<bool> is_paused;
    
    // FFmpeg context for muxing
    AVFormatContext *format_ctx;
    AVStream *video_stream;
    AVStream *audio_stream;
    AVCodecContext *video_codec_ctx;
    
    // Encoder wrappers
    h264_encoder *h264_enc;
    vp9_encoder *vp9_enc;
    av1_encoder *av1_enc;
    
    // Replay buffer
    replay_buffer_t *replay_buffer;
    bool replay_buffer_enabled;
    
    // Frame queues
    std::queue<video_frame_t> video_queue;
    std::queue<audio_chunk_t> audio_queue;
    std::mutex video_mutex;
    std::mutex audio_mutex;
    
    std::thread encoding_thread;
    std::atomic<bool> thread_running;
    
    DiskManager *disk_manager;
    
    uint32_t frame_drop_count;
    uint64_t next_recording_id;
    
public:
    RecordingManager();
    ~RecordingManager();
    
    // Initialization
    int init(const char *output_dir = nullptr);
    
    // Recording control
    int start_recording(enum RecordingPreset preset = PRESET_BALANCED,
                       const char *game_name = nullptr);
    int stop_recording();
    int pause_recording();
    int resume_recording();
    
    // Frame submission
    int submit_video_frame(const uint8_t *frame_data,
                          uint32_t width, uint32_t height,
                          const char *pixel_format,
                          uint64_t timestamp_us);
    
    int submit_audio_chunk(const float *samples,
                          uint32_t sample_count,
                          uint32_t sample_rate,
                          uint64_t timestamp_us);
    
    // Configuration
    int set_output_directory(const char *directory);
    int set_max_storage(uint64_t max_mb);
    int set_auto_cleanup(bool enabled, uint32_t threshold_percent);
    
    // Replay buffer control
    int enable_replay_buffer(uint32_t duration_seconds, uint32_t max_memory_mb);
    int disable_replay_buffer();
    int save_replay_buffer(const char *filename, uint32_t duration_sec);
    int save_replay_buffer(const char *filename, uint32_t duration_sec, enum VideoCodec codec);
    
    // Metadata control
    int add_chapter_marker(const char *title, const char *description);
    int set_game_name(const char *name);
    int add_audio_track(const char *name, uint8_t channels, uint32_t sample_rate);
    
    // Query state
    bool is_recording_active();
    bool is_recording_paused();
    const recording_info_t* get_active_recording();
    
    // Statistics
    uint64_t get_current_file_size();
    uint64_t get_available_disk_space();
    uint32_t get_encoding_queue_depth();
    uint32_t get_frame_drop_count();
    
    void cleanup();
    
private:
    void encoding_thread_main();
    int update_recording_metadata();
    int init_video_encoder(enum VideoCodec codec, uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate_kbps);
    int init_muxer(enum ContainerFormat format);
    int encode_frame_with_active_encoder(const uint8_t *frame_data, uint32_t width, uint32_t height, const char *pixel_format);
    void cleanup_encoders();
};

#endif /* RECORDING_MANAGER_H */
