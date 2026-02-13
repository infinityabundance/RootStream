/* Audio/Video Synchronization Manager for RootStream */
#ifndef AUDIO_SYNC_H
#define AUDIO_SYNC_H

#include <stdint.h>
#include <pthread.h>

class AudioSync {
private:
    uint64_t video_timestamp_us;
    uint64_t audio_timestamp_us;
    int64_t sync_offset_us;
    float playback_speed;
    int sync_threshold_ms;
    
    pthread_mutex_t lock;
    
    int sync_correction_count;
    int64_t total_correction_us;
    
public:
    AudioSync();
    ~AudioSync();
    
    // Initialization
    int init(int sync_threshold_ms = 50);
    
    // Timestamp tracking
    int update_video_timestamp(uint64_t timestamp_us);
    int update_audio_timestamp(uint64_t timestamp_us);
    
    // Sync correction
    int64_t calculate_sync_offset();
    float get_playback_speed_correction();
    
    // State queries
    int64_t get_current_av_offset_us();
    bool is_in_sync();
    
    // Statistics
    int get_sync_correction_count() const { return sync_correction_count; }
    float get_average_correction_ms();
    
    void cleanup();
};

#endif // AUDIO_SYNC_H
