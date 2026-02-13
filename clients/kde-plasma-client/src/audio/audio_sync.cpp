/* Audio/Video Sync Implementation */
#include "audio_sync.h"
#include <cstdio>
#include <cmath>

AudioSync::AudioSync()
    : video_timestamp_us(0), audio_timestamp_us(0), sync_offset_us(0),
      playback_speed(1.0f), sync_threshold_ms(50),
      sync_correction_count(0), total_correction_us(0) {
    pthread_mutex_init(&lock, nullptr);
}

AudioSync::~AudioSync() {
    cleanup();
}

int AudioSync::init(int sync_threshold_ms) {
    this->sync_threshold_ms = sync_threshold_ms;
    video_timestamp_us = 0;
    audio_timestamp_us = 0;
    sync_offset_us = 0;
    playback_speed = 1.0f;
    sync_correction_count = 0;
    total_correction_us = 0;
    return 0;
}

int AudioSync::update_video_timestamp(uint64_t timestamp_us) {
    pthread_mutex_lock(&lock);
    video_timestamp_us = timestamp_us;
    pthread_mutex_unlock(&lock);
    return 0;
}

int AudioSync::update_audio_timestamp(uint64_t timestamp_us) {
    pthread_mutex_lock(&lock);
    audio_timestamp_us = timestamp_us;
    pthread_mutex_unlock(&lock);
    return 0;
}

int64_t AudioSync::calculate_sync_offset() {
    pthread_mutex_lock(&lock);
    
    if (video_timestamp_us == 0 || audio_timestamp_us == 0) {
        pthread_mutex_unlock(&lock);
        return 0;
    }
    
    sync_offset_us = (int64_t)video_timestamp_us - (int64_t)audio_timestamp_us;
    pthread_mutex_unlock(&lock);
    
    return sync_offset_us;
}

float AudioSync::get_playback_speed_correction() {
    pthread_mutex_lock(&lock);
    
    int64_t offset = sync_offset_us;
    int64_t threshold_us = sync_threshold_ms * 1000LL;
    
    // No correction needed if within threshold
    if (std::abs(offset) < threshold_us) {
        playback_speed = 1.0f;
        pthread_mutex_unlock(&lock);
        return playback_speed;
    }
    
    // Apply gentle speed correction (±5% max)
    // Audio is ahead: slow down slightly
    // Audio is behind: speed up slightly
    float correction = (float)offset / (float)(threshold_us * 10);
    correction = std::max(-0.05f, std::min(0.05f, correction));
    
    playback_speed = 1.0f + correction;
    
    sync_correction_count++;
    total_correction_us += std::abs(offset);
    
    pthread_mutex_unlock(&lock);
    return playback_speed;
}

int64_t AudioSync::get_current_av_offset_us() {
    pthread_mutex_lock(&lock);
    int64_t offset = sync_offset_us;
    pthread_mutex_unlock(&lock);
    return offset;
}

bool AudioSync::is_in_sync() {
    pthread_mutex_lock(&lock);
    int64_t threshold_us = sync_threshold_ms * 1000LL;
    bool in_sync = std::abs(sync_offset_us) < threshold_us;
    pthread_mutex_unlock(&lock);
    return in_sync;
}

float AudioSync::get_average_correction_ms() {
    pthread_mutex_lock(&lock);
    if (sync_correction_count == 0) {
        pthread_mutex_unlock(&lock);
        return 0.0f;
    }
    float avg = (float)total_correction_us / (float)sync_correction_count / 1000.0f;
    pthread_mutex_unlock(&lock);
    return avg;
}

void AudioSync::cleanup() {
    pthread_mutex_destroy(&lock);
}
