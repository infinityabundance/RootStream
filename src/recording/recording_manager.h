#ifndef RECORDING_MANAGER_H
#define RECORDING_MANAGER_H

#include "recording_types.h"
#include "disk_manager.h"
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

class RecordingManager {
private:
    struct {
        char output_directory[1024];
        uint64_t max_storage_mb;
        uint32_t auto_cleanup_threshold_percent;
        bool auto_cleanup_enabled;
    } config;
    
    recording_info_t active_recording;
    std::atomic<bool> is_recording;
    std::atomic<bool> is_paused;
    
    // FFmpeg context for muxing
    AVFormatContext *format_ctx;
    AVStream *video_stream;
    AVStream *audio_stream;
    AVCodecContext *video_codec_ctx;
    
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
};

#endif /* RECORDING_MANAGER_H */
