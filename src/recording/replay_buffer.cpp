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
    
    // Create video stream
    AVStream *video_stream = nullptr;
    if (!buffer->video_frames.empty()) {
        const replay_video_frame_t &first_frame = buffer->video_frames.front();
        
        video_stream = avformat_new_stream(fmt_ctx, nullptr);
        if (!video_stream) {
            fprintf(stderr, "Replay Buffer: Failed to create video stream\n");
            avformat_free_context(fmt_ctx);
            return -1;
        }
        
        video_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
        video_stream->codecpar->codec_id = AV_CODEC_ID_H264; // Assume H.264 encoded frames
        video_stream->codecpar->width = first_frame.width;
        video_stream->codecpar->height = first_frame.height;
        video_stream->time_base = (AVRational){1, 1000000}; // Microseconds
    }
    
    // Create audio stream if audio chunks exist
    // TODO: Audio encoding not yet fully implemented
    // Audio stream creation is disabled until proper Opus encoding is added
    AVStream *audio_stream = nullptr;
    /*
    if (!buffer->audio_chunks.empty()) {
        const replay_audio_chunk_t &first_chunk = buffer->audio_chunks.front();
        
        audio_stream = avformat_new_stream(fmt_ctx, nullptr);
        if (!audio_stream) {
            fprintf(stderr, "Replay Buffer: Failed to create audio stream\n");
            avformat_free_context(fmt_ctx);
            return -1;
        }
        
        audio_stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
        audio_stream->codecpar->codec_id = AV_CODEC_ID_OPUS; // Assume Opus encoded audio
        audio_stream->codecpar->ch_layout.nb_channels = first_chunk.channels;
        audio_stream->codecpar->sample_rate = first_chunk.sample_rate;
        audio_stream->time_base = (AVRational){1, 1000000}; // Microseconds
    }
    */
    
    // Open output file
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&fmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Replay Buffer: Could not open output file '%s'\n", filename);
            avformat_free_context(fmt_ctx);
            return -1;
        }
    }
    
    // Write header
    ret = avformat_write_header(fmt_ctx, nullptr);
    if (ret < 0) {
        fprintf(stderr, "Replay Buffer: Error writing header\n");
        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&fmt_ctx->pb);
        }
        avformat_free_context(fmt_ctx);
        return -1;
    }
    
    // Write video and audio frames in timestamp order
    size_t video_idx = 0;
    size_t audio_idx = 0;
    size_t frames_written = 0;
    
    while (video_idx < buffer->video_frames.size() || audio_idx < buffer->audio_chunks.size()) {
        bool write_video = false;
        
        // Determine which packet to write next (video or audio)
        if (video_idx < buffer->video_frames.size() && audio_idx < buffer->audio_chunks.size()) {
            // Both available, choose based on timestamp
            write_video = buffer->video_frames[video_idx].timestamp_us <= 
                         buffer->audio_chunks[audio_idx].timestamp_us;
        } else if (video_idx < buffer->video_frames.size()) {
            write_video = true;
        } else {
            write_video = false;
        }
        
        if (write_video) {
            const auto &frame = buffer->video_frames[video_idx];
            if (frame.timestamp_us >= cutoff_timestamp_us) {
                // Allocate memory for packet data
                uint8_t *data_copy = (uint8_t *)av_memdup(frame.data, frame.size);
                if (!data_copy) {
                    fprintf(stderr, "Replay Buffer: Failed to allocate packet data\n");
                    video_idx++;
                    continue;
                }
                
                AVPacket *pkt = av_packet_alloc();
                if (pkt) {
                    // Use av_packet_from_data to properly manage packet data ownership
                    int ret = av_packet_from_data(pkt, data_copy, frame.size);
                    if (ret < 0) {
                        av_free(data_copy); // Free the allocated memory
                        av_packet_free(&pkt);
                        video_idx++;
                        continue;
                    }
                    
                    pkt->stream_index = video_stream->index;
                    pkt->pts = frame.timestamp_us;
                    pkt->dts = frame.timestamp_us;
                    
                    if (frame.is_keyframe) {
                        pkt->flags |= AV_PKT_FLAG_KEY;
                    }
                    
                    av_interleaved_write_frame(fmt_ctx, pkt);
                    frames_written++;
                    av_packet_free(&pkt);
                }
            }
            video_idx++;
        } else {
            // TODO: Audio encoding not yet implemented
            // Audio chunks in replay buffer are raw float samples that need to be 
            // encoded to Opus before muxing. Skip audio chunks for now.
            // When implementing:
            // 1. Encode float samples to Opus
            // 2. Create packet from encoded data
            // 3. Set proper pts/dts
            // 4. Write packet to muxer
            audio_idx++;
        }
    }
    
    // Write trailer
    av_write_trailer(fmt_ctx);
    
    // Close output file
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&fmt_ctx->pb);
    }
    
    avformat_free_context(fmt_ctx);
    
    printf("Replay Buffer: Saved %zu frames to %s\n", frames_written, filename);
    
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
