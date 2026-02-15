#include "recording_manager.h"
#include "recording_presets.h"
#include "h264_encoder_wrapper.h"
#include "vp9_encoder_wrapper.h"
#include "av1_encoder_wrapper.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sys/stat.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

RecordingManager::RecordingManager() 
    : format_ctx(nullptr), video_stream(nullptr), audio_stream(nullptr),
      video_codec_ctx(nullptr), h264_enc(nullptr), vp9_enc(nullptr), av1_enc(nullptr),
      replay_buffer(nullptr), replay_buffer_enabled(false),
      disk_manager(nullptr), frame_drop_count(0), next_recording_id(1) {
    
    is_recording.store(false);
    is_paused.store(false);
    thread_running.store(false);
    
    memset(&config, 0, sizeof(config));
    memset(&active_recording, 0, sizeof(active_recording));
    memset(&metadata, 0, sizeof(metadata));
    
    config.max_storage_mb = 10000;  // 10GB default
    config.auto_cleanup_threshold_percent = 90;
    config.auto_cleanup_enabled = false;
    
    strncpy(config.output_directory, "recordings", sizeof(config.output_directory) - 1);
}

RecordingManager::~RecordingManager() {
    cleanup();
}

int RecordingManager::init(const char *output_dir) {
    if (output_dir) {
        strncpy(config.output_directory, output_dir, sizeof(config.output_directory) - 1);
    }
    
    // Initialize disk manager
    disk_manager = new DiskManager();
    if (disk_manager->init(config.output_directory, config.max_storage_mb) != 0) {
        fprintf(stderr, "ERROR: Failed to initialize disk manager\n");
        delete disk_manager;
        disk_manager = nullptr;
        return -1;
    }
    
    printf("✓ Recording manager initialized\n");
    printf("  Output directory: %s\n", config.output_directory);
    printf("  Max storage: %lu MB\n", config.max_storage_mb);
    printf("  Available space: %lu MB\n", disk_manager->get_free_space_mb());
    
    return 0;
}

int RecordingManager::start_recording(enum RecordingPreset preset, const char *game_name) {
    if (is_recording.load()) {
        fprintf(stderr, "ERROR: Recording already in progress\n");
        return -1;
    }
    
    // Check disk space
    if (disk_manager && disk_manager->is_space_low()) {
        fprintf(stderr, "WARNING: Low disk space, attempting cleanup\n");
        if (config.auto_cleanup_enabled) {
            disk_manager->auto_cleanup_old_recordings();
        }
    }
    
    if (disk_manager && disk_manager->is_at_limit()) {
        fprintf(stderr, "ERROR: Storage limit reached\n");
        return -1;
    }
    
    // Get preset configuration
    const struct RecordingPresetConfig *preset_cfg = get_recording_preset(preset);
    
    // Initialize recording info
    memset(&active_recording, 0, sizeof(active_recording));
    active_recording.recording_id = next_recording_id++;
    active_recording.preset = preset;
    active_recording.video_codec = preset_cfg->video_codec;
    active_recording.audio_codec = preset_cfg->audio_codec;
    active_recording.container = preset_cfg->container;
    active_recording.creation_time_us = (uint64_t)time(nullptr) * 1000000ULL;
    active_recording.start_time_us = active_recording.creation_time_us;
    
    // Generate filename
    if (disk_manager) {
        std::string filename = disk_manager->generate_filename(game_name);
        strncpy(active_recording.filename, filename.c_str(), sizeof(active_recording.filename) - 1);
        
        std::string filepath = std::string(config.output_directory) + "/" + filename;
        strncpy(active_recording.filepath, filepath.c_str(), sizeof(active_recording.filepath) - 1);
    } else {
        snprintf(active_recording.filename, sizeof(active_recording.filename), 
                "recording_%lu.mp4", (unsigned long)time(nullptr));
        snprintf(active_recording.filepath, sizeof(active_recording.filepath),
                "%s/%s", config.output_directory, active_recording.filename);
    }
    
    if (game_name) {
        strncpy(active_recording.metadata, game_name, sizeof(active_recording.metadata) - 1);
    }
    
    // Initialize muxer
    if (init_muxer(preset_cfg->container) != 0) {
        fprintf(stderr, "ERROR: Failed to initialize muxer\n");
        return -1;
    }
    
    // Initialize video encoder
    // Note: Using default 1920x1080 @ 60fps for now
    // In a full implementation, these would come from capture settings
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t fps = 60;
    uint32_t bitrate_kbps = (preset_cfg->video_codec == VIDEO_CODEC_H264) ? 
                            preset_cfg->h264_bitrate_kbps : 
                            (preset_cfg->video_codec == VIDEO_CODEC_VP9) ?
                            preset_cfg->vp9_bitrate_kbps :
                            preset_cfg->av1_bitrate_kbps;
    
    if (init_video_encoder(preset_cfg->video_codec, width, height, fps, bitrate_kbps) != 0) {
        fprintf(stderr, "ERROR: Failed to initialize video encoder\n");
        // Cleanup muxer on failure
        if (format_ctx) {
            if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
                avio_closep(&format_ctx->pb);
            }
            avformat_free_context(format_ctx);
            format_ctx = nullptr;
        }
        return -1;
    }
    
    is_recording.store(true);
    is_paused.store(false);
    
    printf("✓ Recording started: %s\n", active_recording.filename);
    printf("  Preset: %s\n", preset_cfg->description);
    printf("  Container: %s\n", preset_cfg->container == CONTAINER_MP4 ? "MP4" : "Matroska");
    
    return 0;
}

int RecordingManager::stop_recording() {
    if (!is_recording.load()) {
        return -1;
    }
    
    is_recording.store(false);
    is_paused.store(false);
    
    // Flush and cleanup encoders before finalizing muxer
    cleanup_encoders();
    
    // Finalize muxer
    if (format_ctx) {
        av_write_trailer(format_ctx);
        
        if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&format_ctx->pb);
        }
        
        avformat_free_context(format_ctx);
        format_ctx = nullptr;
        video_stream = nullptr;
        audio_stream = nullptr;
    }
    
    if (video_codec_ctx) {
        avcodec_free_context(&video_codec_ctx);
        video_codec_ctx = nullptr;
    }
    
    // Update recording info
    active_recording.is_complete = true;
    active_recording.duration_us = (uint64_t)time(nullptr) * 1000000ULL - active_recording.start_time_us;
    
    // Get file size
    struct stat st;
    if (stat(active_recording.filepath, &st) == 0) {
        active_recording.file_size_bytes = st.st_size;
    }
    
    printf("✓ Recording stopped: %s\n", active_recording.filename);
    printf("  Duration: %.1f seconds\n", active_recording.duration_us / 1000000.0);
    printf("  Size: %.2f MB\n", active_recording.file_size_bytes / (1024.0 * 1024.0));
    
    return 0;
}

int RecordingManager::pause_recording() {
    if (!is_recording.load() || is_paused.load()) {
        return -1;
    }
    
    is_paused.store(true);
    active_recording.is_paused = true;
    
    printf("✓ Recording paused\n");
    return 0;
}

int RecordingManager::resume_recording() {
    if (!is_recording.load() || !is_paused.load()) {
        return -1;
    }
    
    is_paused.store(false);
    active_recording.is_paused = false;
    
    printf("✓ Recording resumed\n");
    return 0;
}

int RecordingManager::submit_video_frame(const uint8_t *frame_data,
                                        uint32_t width, uint32_t height,
                                        const char *pixel_format,
                                        uint64_t timestamp_us) {
    if (!is_recording.load() || is_paused.load()) {
        // Still add to replay buffer even if not recording
        if (replay_buffer_enabled && replay_buffer && frame_data) {
            // TODO: Encode frame before adding to replay buffer
            // For now, we would need the encoded frame data
            // This will be fully integrated when the encoding thread is implemented
        }
        return 0;
    }
    
    if (!frame_data) {
        return -1;
    }
    
    // Check queue size
    {
        std::lock_guard<std::mutex> lock(video_mutex);
        if (video_queue.size() >= MAX_RECORDING_QUEUE_SIZE) {
            frame_drop_count++;
            fprintf(stderr, "WARNING: Video queue full, dropping frame\n");
            return -1;
        }
    }
    
    // TODO: Actual encoding would happen in encoding thread
    // Once frames are encoded, they should be added to replay buffer:
    // if (replay_buffer_enabled && replay_buffer) {
    //     replay_buffer_add_video_frame(replay_buffer, encoded_data, encoded_size,
    //                                   width, height, timestamp_us, is_keyframe);
    // }
    
    return 0;
}

int RecordingManager::submit_audio_chunk(const float *samples,
                                        uint32_t sample_count,
                                        uint32_t sample_rate,
                                        uint64_t timestamp_us) {
    if (!is_recording.load() || is_paused.load()) {
        return 0;
    }
    
    if (!samples) {
        return -1;
    }
    
    // Check queue size
    {
        std::lock_guard<std::mutex> lock(audio_mutex);
        if (audio_queue.size() >= MAX_RECORDING_QUEUE_SIZE) {
            fprintf(stderr, "WARNING: Audio queue full, dropping chunk\n");
            return -1;
        }
    }
    
    // For now, just count chunks (actual encoding would happen in encoding thread)
    
    return 0;
}

int RecordingManager::set_output_directory(const char *directory) {
    if (!directory) return -1;
    
    strncpy(config.output_directory, directory, sizeof(config.output_directory) - 1);
    
    if (disk_manager) {
        return disk_manager->init(directory, config.max_storage_mb);
    }
    
    return 0;
}

int RecordingManager::set_max_storage(uint64_t max_mb) {
    config.max_storage_mb = max_mb;
    
    if (disk_manager) {
        return disk_manager->init(config.output_directory, max_mb);
    }
    
    return 0;
}

int RecordingManager::set_auto_cleanup(bool enabled, uint32_t threshold_percent) {
    config.auto_cleanup_enabled = enabled;
    config.auto_cleanup_threshold_percent = threshold_percent;
    return 0;
}

bool RecordingManager::is_recording_active() {
    return is_recording.load();
}

bool RecordingManager::is_recording_paused() {
    return is_paused.load();
}

const recording_info_t* RecordingManager::get_active_recording() {
    if (is_recording.load()) {
        return &active_recording;
    }
    return nullptr;
}

uint64_t RecordingManager::get_current_file_size() {
    if (!is_recording.load()) return 0;
    
    struct stat st;
    if (stat(active_recording.filepath, &st) == 0) {
        return st.st_size;
    }
    
    return 0;
}

uint64_t RecordingManager::get_available_disk_space() {
    if (disk_manager) {
        return disk_manager->get_free_space_mb();
    }
    return 0;
}

uint32_t RecordingManager::get_encoding_queue_depth() {
    std::lock_guard<std::mutex> lock(video_mutex);
    return video_queue.size();
}

uint32_t RecordingManager::get_frame_drop_count() {
    return frame_drop_count;
}

void RecordingManager::cleanup() {
    if (is_recording.load()) {
        stop_recording();
    }
    
    if (thread_running.load()) {
        thread_running.store(false);
        if (encoding_thread.joinable()) {
            encoding_thread.join();
        }
    }
    
    if (replay_buffer_enabled && replay_buffer) {
        disable_replay_buffer();
    }
    
    cleanup_encoders();
    
    if (disk_manager) {
        delete disk_manager;
        disk_manager = nullptr;
    }
}

void RecordingManager::encoding_thread_main() {
    // Placeholder for encoding thread
    // Would process video_queue and audio_queue here
}

int RecordingManager::update_recording_metadata() {
    // Update duration
    active_recording.duration_us = (uint64_t)time(nullptr) * 1000000ULL - active_recording.start_time_us;
    
    // Update file size
    struct stat st;
    if (stat(active_recording.filepath, &st) == 0) {
        active_recording.file_size_bytes = st.st_size;
    }
    
    return 0;
}

int RecordingManager::init_video_encoder(enum VideoCodec codec, uint32_t width, uint32_t height, 
                                        uint32_t fps, uint32_t bitrate_kbps) {
    // Get preset configuration
    const struct RecordingPresetConfig *preset_cfg = get_recording_preset(active_recording.preset);
    
    // Initialize the appropriate encoder based on codec type
    switch (codec) {
        case VIDEO_CODEC_H264: {
            // Check if H.264 encoder is available
            if (!h264_encoder_available()) {
                fprintf(stderr, "ERROR: H.264 encoder not available\n");
                return -1;
            }
            
            // Allocate encoder
            h264_enc = (h264_encoder *)malloc(sizeof(h264_encoder));
            if (!h264_enc) {
                fprintf(stderr, "ERROR: Failed to allocate H.264 encoder\n");
                return -1;
            }
            memset(h264_enc, 0, sizeof(h264_encoder));
            
            // Initialize H.264 encoder
            int ret = h264_encoder_init(h264_enc, width, height, fps, bitrate_kbps,
                                       preset_cfg->h264_preset, preset_cfg->h264_crf);
            if (ret != 0) {
                fprintf(stderr, "ERROR: Failed to initialize H.264 encoder\n");
                free(h264_enc);
                h264_enc = nullptr;
                return -1;
            }
            
            printf("✓ H.264 encoder initialized: %ux%u @ %u fps, preset=%s, bitrate=%u kbps\n",
                   width, height, fps, preset_cfg->h264_preset, bitrate_kbps);
            break;
        }
        
        case VIDEO_CODEC_VP9: {
            // Check if VP9 encoder is available
            if (!vp9_encoder_available()) {
                fprintf(stderr, "ERROR: VP9 encoder not available\n");
                return -1;
            }
            
            // Allocate encoder
            vp9_enc = (vp9_encoder *)malloc(sizeof(vp9_encoder));
            if (!vp9_enc) {
                fprintf(stderr, "ERROR: Failed to allocate VP9 encoder\n");
                return -1;
            }
            memset(vp9_enc, 0, sizeof(vp9_encoder));
            
            // Initialize VP9 encoder
            // Use -1 for quality to enable bitrate mode
            int ret = vp9_encoder_init(vp9_enc, width, height, fps, bitrate_kbps,
                                      preset_cfg->vp9_cpu_used, -1);
            if (ret != 0) {
                fprintf(stderr, "ERROR: Failed to initialize VP9 encoder\n");
                free(vp9_enc);
                vp9_enc = nullptr;
                return -1;
            }
            
            printf("✓ VP9 encoder initialized: %ux%u @ %u fps, cpu_used=%d, bitrate=%u kbps\n",
                   width, height, fps, preset_cfg->vp9_cpu_used, bitrate_kbps);
            break;
        }
        
        case VIDEO_CODEC_AV1: {
            // Check if AV1 encoder is available
            if (!av1_encoder_available()) {
                fprintf(stderr, "ERROR: AV1 encoder not available\n");
                return -1;
            }
            
            // Allocate encoder
            av1_enc = (av1_encoder *)malloc(sizeof(av1_encoder));
            if (!av1_enc) {
                fprintf(stderr, "ERROR: Failed to allocate AV1 encoder\n");
                return -1;
            }
            memset(av1_enc, 0, sizeof(av1_encoder));
            
            // Initialize AV1 encoder
            // Use -1 for CRF to enable bitrate mode
            int ret = av1_encoder_init(av1_enc, width, height, fps, bitrate_kbps,
                                      preset_cfg->av1_cpu_used, -1);
            if (ret != 0) {
                fprintf(stderr, "ERROR: Failed to initialize AV1 encoder\n");
                free(av1_enc);
                av1_enc = nullptr;
                return -1;
            }
            
            printf("✓ AV1 encoder initialized: %ux%u @ %u fps, cpu_used=%d, bitrate=%u kbps\n",
                   width, height, fps, preset_cfg->av1_cpu_used, bitrate_kbps);
            break;
        }
        
        default:
            fprintf(stderr, "ERROR: Unknown video codec: %d\n", codec);
            return -1;
    }
    
    // Create video stream in the muxer
    if (!format_ctx) {
        fprintf(stderr, "ERROR: Format context not initialized\n");
        return -1;
    }
    
    video_stream = avformat_new_stream(format_ctx, nullptr);
    if (!video_stream) {
        fprintf(stderr, "ERROR: Failed to create video stream\n");
        return -1;
    }
    
    // Set stream parameters based on codec
    video_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    video_stream->codecpar->width = width;
    video_stream->codecpar->height = height;
    video_stream->time_base = (AVRational){1, (int)fps};
    video_stream->avg_frame_rate = (AVRational){(int)fps, 1};
    
    switch (codec) {
        case VIDEO_CODEC_H264:
            video_stream->codecpar->codec_id = AV_CODEC_ID_H264;
            break;
        case VIDEO_CODEC_VP9:
            video_stream->codecpar->codec_id = AV_CODEC_ID_VP9;
            break;
        case VIDEO_CODEC_AV1:
            video_stream->codecpar->codec_id = AV_CODEC_ID_AV1;
            break;
    }
    
    // Store video parameters in recording info
    active_recording.video_width = width;
    active_recording.video_height = height;
    active_recording.video_fps = fps;
    active_recording.video_bitrate_kbps = bitrate_kbps;
    
    return 0;
}

int RecordingManager::init_muxer(enum ContainerFormat format) {
    const char *format_name = (format == CONTAINER_MP4) ? "mp4" : "matroska";
    
    int ret = avformat_alloc_output_context2(&format_ctx, nullptr, format_name, active_recording.filepath);
    if (ret < 0 || !format_ctx) {
        fprintf(stderr, "ERROR: Could not create output context\n");
        return -1;
    }
    
    // Open output file
    if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&format_ctx->pb, active_recording.filepath, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "ERROR: Could not open output file '%s'\n", active_recording.filepath);
            avformat_free_context(format_ctx);
            format_ctx = nullptr;
            return -1;
        }
    }
    
    // Write header
    ret = avformat_write_header(format_ctx, nullptr);
    if (ret < 0) {
        fprintf(stderr, "ERROR: Error writing header\n");
        if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&format_ctx->pb);
        }
        avformat_free_context(format_ctx);
        format_ctx = nullptr;
        return -1;
    }
    
    return 0;
}

int RecordingManager::encode_frame_with_active_encoder(const uint8_t *frame_data, 
                                                       uint32_t width, uint32_t height, 
                                                       const char *pixel_format) {
    if (!frame_data || !pixel_format) {
        return -1;
    }
    
    uint8_t *encoded_data = nullptr;
    size_t encoded_size = 0;
    bool is_keyframe = false;
    int ret = -1;
    
    // Encode frame with the appropriate encoder
    switch (active_recording.video_codec) {
        case VIDEO_CODEC_H264:
            if (!h264_enc || !h264_enc->initialized) {
                fprintf(stderr, "ERROR: H.264 encoder not initialized\n");
                return -1;
            }
            ret = h264_encoder_encode_frame(h264_enc, frame_data, pixel_format,
                                           &encoded_data, &encoded_size, &is_keyframe);
            break;
            
        case VIDEO_CODEC_VP9:
            if (!vp9_enc || !vp9_enc->initialized) {
                fprintf(stderr, "ERROR: VP9 encoder not initialized\n");
                return -1;
            }
            ret = vp9_encoder_encode_frame(vp9_enc, frame_data, pixel_format,
                                          &encoded_data, &encoded_size, &is_keyframe);
            break;
            
        case VIDEO_CODEC_AV1:
            if (!av1_enc || !av1_enc->initialized) {
                fprintf(stderr, "ERROR: AV1 encoder not initialized\n");
                return -1;
            }
            ret = av1_encoder_encode_frame(av1_enc, frame_data, pixel_format,
                                          &encoded_data, &encoded_size, &is_keyframe);
            break;
            
        default:
            fprintf(stderr, "ERROR: Unknown video codec\n");
            return -1;
    }
    
    if (ret != 0 || !encoded_data || encoded_size == 0) {
        // Encoding failed or no output (some encoders may delay output)
        return ret;
    }
    
    // Create packet from encoded data
    if (!format_ctx || !video_stream) {
        fprintf(stderr, "ERROR: Format context or video stream not initialized\n");
        return -1;
    }
    
    // Allocate memory for packet data
    uint8_t *packet_data = (uint8_t *)av_memdup(encoded_data, encoded_size);
    if (!packet_data) {
        fprintf(stderr, "ERROR: Failed to allocate packet data\n");
        return -1;
    }
    
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "ERROR: Failed to allocate packet\n");
        av_free(packet_data);
        return -1;
    }
    
    // Set packet data
    ret = av_packet_from_data(pkt, packet_data, encoded_size);
    if (ret < 0) {
        fprintf(stderr, "ERROR: Failed to create packet from data\n");
        av_free(packet_data);
        av_packet_free(&pkt);
        return -1;
    }
    
    // Set packet properties
    pkt->stream_index = video_stream->index;
    pkt->pts = active_recording.duration_us;
    pkt->dts = active_recording.duration_us;
    
    if (is_keyframe) {
        pkt->flags |= AV_PKT_FLAG_KEY;
    }
    
    // Write packet to muxer
    ret = av_interleaved_write_frame(format_ctx, pkt);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "ERROR: Failed to write frame: %s\n", errbuf);
    }
    
    av_packet_free(&pkt);
    
    return ret;
}

void RecordingManager::cleanup_encoders() {
    // Cleanup H.264 encoder
    if (h264_enc) {
        if (h264_enc->initialized) {
            h264_encoder_flush(h264_enc);
            h264_encoder_cleanup(h264_enc);
        }
        free(h264_enc);
        h264_enc = nullptr;
    }
    
    // Cleanup VP9 encoder
    if (vp9_enc) {
        if (vp9_enc->initialized) {
            vp9_encoder_flush(vp9_enc);
            vp9_encoder_cleanup(vp9_enc);
        }
        free(vp9_enc);
        vp9_enc = nullptr;
    }
    
    // Cleanup AV1 encoder
    if (av1_enc) {
        if (av1_enc->initialized) {
            av1_encoder_flush(av1_enc);
            av1_encoder_cleanup(av1_enc);
        }
        free(av1_enc);
        av1_enc = nullptr;
    }
}

// Replay buffer methods
int RecordingManager::enable_replay_buffer(uint32_t duration_seconds, uint32_t max_memory_mb) {
    if (replay_buffer_enabled) {
        fprintf(stderr, "ERROR: Replay buffer already enabled\n");
        return -1;
    }
    
    replay_buffer = replay_buffer_create(duration_seconds, max_memory_mb);
    if (!replay_buffer) {
        fprintf(stderr, "ERROR: Failed to create replay buffer\n");
        return -1;
    }
    
    replay_buffer_enabled = true;
    printf("✓ Replay buffer enabled: %u seconds, max memory: %u MB\n", 
           duration_seconds, max_memory_mb);
    
    return 0;
}

int RecordingManager::disable_replay_buffer() {
    if (!replay_buffer_enabled) {
        return 0;
    }
    
    if (replay_buffer) {
        replay_buffer_destroy(replay_buffer);
        replay_buffer = nullptr;
    }
    
    replay_buffer_enabled = false;
    printf("✓ Replay buffer disabled\n");
    
    return 0;
}

int RecordingManager::save_replay_buffer(const char *filename, uint32_t duration_sec) {
    if (!replay_buffer_enabled || !replay_buffer) {
        fprintf(stderr, "ERROR: Replay buffer not enabled\n");
        return -1;
    }
    
    if (!filename) {
        fprintf(stderr, "ERROR: Invalid filename\n");
        return -1;
    }
    
    // Generate full filepath
    char filepath[2048];
    if (filename[0] == '/') {
        // Absolute path
        strncpy(filepath, filename, sizeof(filepath) - 1);
    } else {
        // Relative to output directory
        snprintf(filepath, sizeof(filepath), "%s/%s", config.output_directory, filename);
    }
    
    // Determine codec to use
    // If recording is active, use the active codec
    // Otherwise, use H.264 as default
    enum VideoCodec codec = VIDEO_CODEC_H264;
    if (is_recording.load()) {
        codec = active_recording.video_codec;
        printf("Saving replay buffer with active recording codec: %d\n", codec);
    } else {
        printf("Saving replay buffer with default codec: H.264\n");
    }
    
    printf("Saving replay buffer to: %s\n", filepath);
    
    int ret = replay_buffer_save(replay_buffer, filepath, duration_sec, codec);
    if (ret != 0) {
        fprintf(stderr, "ERROR: Failed to save replay buffer\n");
        return -1;
    }
    
    printf("✓ Replay buffer saved successfully\n");
    return 0;
}

int RecordingManager::save_replay_buffer(const char *filename, uint32_t duration_sec, enum VideoCodec codec) {
    if (!replay_buffer_enabled || !replay_buffer) {
        fprintf(stderr, "ERROR: Replay buffer not enabled\n");
        return -1;
    }
    
    if (!filename) {
        fprintf(stderr, "ERROR: Invalid filename\n");
        return -1;
    }
    
    // Generate full filepath
    char filepath[2048];
    if (filename[0] == '/') {
        // Absolute path
        strncpy(filepath, filename, sizeof(filepath) - 1);
    } else {
        // Relative to output directory
        snprintf(filepath, sizeof(filepath), "%s/%s", config.output_directory, filename);
    }
    
    printf("Saving replay buffer to: %s with specified codec: %d\n", filepath, codec);
    
    int ret = replay_buffer_save(replay_buffer, filepath, duration_sec, codec);
    if (ret != 0) {
        fprintf(stderr, "ERROR: Failed to save replay buffer\n");
        return -1;
    }
    
    printf("✓ Replay buffer saved successfully\n");
    return 0;
}

// Metadata methods
int RecordingManager::add_chapter_marker(const char *title, const char *description) {
    if (!is_recording.load()) {
        fprintf(stderr, "ERROR: Not recording\n");
        return -1;
    }
    
    if (!title) {
        return -1;
    }
    
    // Add chapter marker to metadata
    if (metadata.marker_count >= MAX_CHAPTER_MARKERS) {
        fprintf(stderr, "WARNING: Maximum chapter markers reached\n");
        return -1;
    }
    
    chapter_marker_t *marker = &metadata.markers[metadata.marker_count];
    marker->timestamp_us = (uint64_t)time(nullptr) * 1000000ULL - active_recording.start_time_us;
    strncpy(marker->title, title, sizeof(marker->title) - 1);
    
    if (description) {
        strncpy(marker->description, description, sizeof(marker->description) - 1);
    }
    
    metadata.marker_count++;
    
    printf("✓ Chapter marker added: %s at %.1fs\n", title, marker->timestamp_us / 1000000.0);
    
    return 0;
}

int RecordingManager::set_game_name(const char *name) {
    if (!name) {
        return -1;
    }
    
    strncpy(metadata.game_name, name, sizeof(metadata.game_name) - 1);
    
    if (is_recording.load()) {
        strncpy(active_recording.metadata, name, sizeof(active_recording.metadata) - 1);
    }
    
    printf("✓ Game name set: %s\n", name);
    
    return 0;
}

int RecordingManager::add_audio_track(const char *name, uint8_t channels, uint32_t sample_rate) {
    if (!name) {
        return -1;
    }
    
    if (metadata.track_count >= MAX_AUDIO_TRACKS) {
        fprintf(stderr, "WARNING: Maximum audio tracks reached\n");
        return -1;
    }
    
    audio_track_info_t *track = &metadata.tracks[metadata.track_count];
    track->track_id = metadata.track_count;
    strncpy(track->name, name, sizeof(track->name) - 1);
    track->channels = channels;
    track->sample_rate = sample_rate;
    track->enabled = true;
    track->volume = 1.0f;
    
    metadata.track_count++;
    
    printf("✓ Audio track added: %s (%u ch, %u Hz)\n", name, channels, sample_rate);
    
    return 0;
}
