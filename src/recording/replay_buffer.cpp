#include "replay_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <deque>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

struct replay_buffer {
    std::deque<replay_video_frame_t> video_frames;
    std::deque<replay_audio_chunk_t> audio_chunks;
    
    uint32_t duration_seconds;
    uint32_t max_memory_mb;
    
    uint64_t total_memory_bytes;
    uint64_t oldest_timestamp_us;
    uint64_t newest_timestamp_us;
    
    std::mutex video_mutex;
    std::mutex audio_mutex;
};

replay_buffer_t* replay_buffer_create(uint32_t duration_seconds, uint32_t max_memory_mb) {
    if (duration_seconds == 0 || duration_seconds > MAX_REPLAY_BUFFER_SECONDS) {
        fprintf(stderr, "Replay Buffer: Invalid duration: %u (max: %u)\n",
                duration_seconds, MAX_REPLAY_BUFFER_SECONDS);
        return nullptr;
    }
    
    replay_buffer_t *buffer = new replay_buffer_t;
    if (!buffer) {
        fprintf(stderr, "Replay Buffer: Failed to allocate buffer\n");
        return nullptr;
    }
    
    buffer->duration_seconds = duration_seconds;
    buffer->max_memory_mb = max_memory_mb;
    buffer->total_memory_bytes = 0;
    buffer->oldest_timestamp_us = 0;
    buffer->newest_timestamp_us = 0;
    
    printf("Replay Buffer created: %u seconds, max memory: %u MB\n",
           duration_seconds, max_memory_mb);
    
    return buffer;
}

static void cleanup_old_frames(replay_buffer_t *buffer, uint64_t current_timestamp_us) {
    if (!buffer) return;
    
    uint64_t max_age_us = (uint64_t)buffer->duration_seconds * 1000000;
    
    // Remove old video frames
    while (!buffer->video_frames.empty()) {
        const auto &oldest = buffer->video_frames.front();
        if (current_timestamp_us - oldest.timestamp_us > max_age_us) {
            buffer->total_memory_bytes -= oldest.size;
            free(oldest.data);
            buffer->video_frames.pop_front();
        } else {
            break;
        }
    }
    
    // Remove old audio chunks
    while (!buffer->audio_chunks.empty()) {
        const auto &oldest = buffer->audio_chunks.front();
        if (current_timestamp_us - oldest.timestamp_us > max_age_us) {
            buffer->total_memory_bytes -= oldest.sample_count * sizeof(float);
            free(oldest.samples);
            buffer->audio_chunks.pop_front();
        } else {
            break;
        }
    }
    
    // Update oldest timestamp
    if (!buffer->video_frames.empty()) {
        buffer->oldest_timestamp_us = buffer->video_frames.front().timestamp_us;
    } else if (!buffer->audio_chunks.empty()) {
        buffer->oldest_timestamp_us = buffer->audio_chunks.front().timestamp_us;
    }
}

static void enforce_memory_limit(replay_buffer_t *buffer) {
    if (!buffer || buffer->max_memory_mb == 0) return;
    
    uint64_t max_memory_bytes = (uint64_t)buffer->max_memory_mb * 1024 * 1024;
    
    // Remove oldest frames until we're under the limit
    while (buffer->total_memory_bytes > max_memory_bytes &&
           !buffer->video_frames.empty()) {
        const auto &oldest = buffer->video_frames.front();
        buffer->total_memory_bytes -= oldest.size;
        free(oldest.data);
        buffer->video_frames.pop_front();
    }
    
    // Remove oldest audio if still over limit
    while (buffer->total_memory_bytes > max_memory_bytes &&
           !buffer->audio_chunks.empty()) {
        const auto &oldest = buffer->audio_chunks.front();
        buffer->total_memory_bytes -= oldest.sample_count * sizeof(float);
        free(oldest.samples);
        buffer->audio_chunks.pop_front();
    }
}

int replay_buffer_add_video_frame(replay_buffer_t *buffer,
                                  const uint8_t *frame_data,
                                  size_t size,
                                  uint32_t width,
                                  uint32_t height,
                                  uint64_t timestamp_us,
                                  bool is_keyframe) {
    if (!buffer || !frame_data || size == 0) {
        return -1;
    }
    
    std::lock_guard<std::mutex> lock(buffer->video_mutex);
    
    // Allocate and copy frame data
    uint8_t *data_copy = (uint8_t *)malloc(size);
    if (!data_copy) {
        fprintf(stderr, "Replay Buffer: Failed to allocate frame memory\n");
        return -1;
    }
    memcpy(data_copy, frame_data, size);
    
    // Create frame entry
    replay_video_frame_t frame;
    frame.data = data_copy;
    frame.size = size;
    frame.timestamp_us = timestamp_us;
    frame.width = width;
    frame.height = height;
    frame.is_keyframe = is_keyframe;
    
    // Add to buffer
    buffer->video_frames.push_back(frame);
    buffer->total_memory_bytes += size;
    buffer->newest_timestamp_us = timestamp_us;
    
    // Cleanup old frames based on time
    cleanup_old_frames(buffer, timestamp_us);
    
    // Enforce memory limit
    enforce_memory_limit(buffer);
    
    return 0;
}

int replay_buffer_add_audio_chunk(replay_buffer_t *buffer,
                                  const float *samples,
                                  size_t sample_count,
                                  uint32_t sample_rate,
                                  uint8_t channels,
                                  uint64_t timestamp_us) {
    if (!buffer || !samples || sample_count == 0) {
        return -1;
    }
    
    std::lock_guard<std::mutex> lock(buffer->audio_mutex);
    
    // Allocate and copy audio data
    size_t data_size = sample_count * sizeof(float);
    float *samples_copy = (float *)malloc(data_size);
    if (!samples_copy) {
        fprintf(stderr, "Replay Buffer: Failed to allocate audio memory\n");
        return -1;
    }
    memcpy(samples_copy, samples, data_size);
    
    // Create audio chunk entry
    replay_audio_chunk_t chunk;
    chunk.samples = samples_copy;
    chunk.sample_count = sample_count;
    chunk.timestamp_us = timestamp_us;
    chunk.sample_rate = sample_rate;
    chunk.channels = channels;
    
    // Add to buffer
    buffer->audio_chunks.push_back(chunk);
    buffer->total_memory_bytes += data_size;
    buffer->newest_timestamp_us = timestamp_us;
    
    // Cleanup old chunks based on time
    cleanup_old_frames(buffer, timestamp_us);
    
    // Enforce memory limit
    enforce_memory_limit(buffer);
    
    return 0;
}

int replay_buffer_save(replay_buffer_t *buffer,
                       const char *filename,
                       uint32_t duration_sec) {
    if (!buffer || !filename) {
        return -1;
    }
    
    std::lock_guard<std::mutex> video_lock(buffer->video_mutex);
    std::lock_guard<std::mutex> audio_lock(buffer->audio_mutex);
    
    if (buffer->video_frames.empty()) {
        fprintf(stderr, "Replay Buffer: No video frames to save\n");
        return -1;
    }
    
    printf("Replay Buffer: Saving %u frames to '%s'\n",
           (uint32_t)buffer->video_frames.size(), filename);
    
    // Calculate time range to save
    uint64_t save_duration_us;
    if (duration_sec == 0) {
        save_duration_us = buffer->newest_timestamp_us - buffer->oldest_timestamp_us;
    } else {
        save_duration_us = (uint64_t)duration_sec * 1000000;
    }
    
    uint64_t cutoff_timestamp_us = buffer->newest_timestamp_us - save_duration_us;
    
    // Initialize FFmpeg output
    AVFormatContext *fmt_ctx = nullptr;
    int ret = avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, filename);
    if (ret < 0) {
        fprintf(stderr, "Replay Buffer: Failed to allocate output context\n");
        return -1;
    }
    
    // TODO: Add video stream setup and muxing
    // This is a simplified implementation - full muxing would require:
    // 1. Creating video and audio streams
    // 2. Writing header
    // 3. Muxing frames in timestamp order
    // 4. Writing trailer
    
    // For now, just write raw frames (simplified)
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Replay Buffer: Failed to open output file\n");
        avformat_free_context(fmt_ctx);
        return -1;
    }
    
    size_t frames_written = 0;
    for (const auto &frame : buffer->video_frames) {
        if (frame.timestamp_us >= cutoff_timestamp_us) {
            fwrite(frame.data, 1, frame.size, f);
            frames_written++;
        }
    }
    
    fclose(f);
    avformat_free_context(fmt_ctx);
    
    printf("Replay Buffer: Saved %zu frames\n", frames_written);
    
    return 0;
}

void replay_buffer_clear(replay_buffer_t *buffer) {
    if (!buffer) return;
    
    std::lock_guard<std::mutex> video_lock(buffer->video_mutex);
    std::lock_guard<std::mutex> audio_lock(buffer->audio_mutex);
    
    // Free all video frames
    for (auto &frame : buffer->video_frames) {
        free(frame.data);
    }
    buffer->video_frames.clear();
    
    // Free all audio chunks
    for (auto &chunk : buffer->audio_chunks) {
        free(chunk.samples);
    }
    buffer->audio_chunks.clear();
    
    buffer->total_memory_bytes = 0;
    buffer->oldest_timestamp_us = 0;
    buffer->newest_timestamp_us = 0;
}

int replay_buffer_get_stats(replay_buffer_t *buffer,
                           uint32_t *video_frames_out,
                           uint32_t *audio_chunks_out,
                           uint32_t *memory_used_mb,
                           uint32_t *duration_sec_out) {
    if (!buffer) {
        return -1;
    }
    
    std::lock_guard<std::mutex> video_lock(buffer->video_mutex);
    std::lock_guard<std::mutex> audio_lock(buffer->audio_mutex);
    
    if (video_frames_out) {
        *video_frames_out = buffer->video_frames.size();
    }
    
    if (audio_chunks_out) {
        *audio_chunks_out = buffer->audio_chunks.size();
    }
    
    if (memory_used_mb) {
        *memory_used_mb = buffer->total_memory_bytes / (1024 * 1024);
    }
    
    if (duration_sec_out) {
        if (buffer->newest_timestamp_us > buffer->oldest_timestamp_us) {
            *duration_sec_out = (buffer->newest_timestamp_us - buffer->oldest_timestamp_us) / 1000000;
        } else {
            *duration_sec_out = 0;
        }
    }
    
    return 0;
}

void replay_buffer_destroy(replay_buffer_t *buffer) {
    if (!buffer) return;
    
    replay_buffer_clear(buffer);
    delete buffer;
}
