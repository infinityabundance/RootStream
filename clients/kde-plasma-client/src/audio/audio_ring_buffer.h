/* Audio Ring Buffer (Jitter Buffer) for RootStream */
#ifndef AUDIO_RING_BUFFER_H
#define AUDIO_RING_BUFFER_H

#include <stddef.h>
#include <pthread.h>
#include <stdint.h>

class AudioRingBuffer {
private:
    float *buffer;
    size_t buffer_size;      // Total capacity in samples
    volatile size_t write_pos;
    volatile size_t read_pos;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    
    int sample_rate;
    int channels;
    int buffer_duration_ms;
    
    bool underrun_flag;
    bool overrun_flag;
    
public:
    AudioRingBuffer();
    ~AudioRingBuffer();
    
    // Initialization
    int init(int sample_rate, int channels, int buffer_duration_ms);
    
    // Writing (from decoder)
    int write_samples(const float *samples, int sample_count,
                     int timeout_ms = 0);
    
    // Reading (for playback)
    int read_samples(float *output, int sample_count,
                    int timeout_ms = 100);
    
    // State queries
    int get_available_samples();
    int get_free_samples();
    float get_fill_percentage();
    int get_latency_ms();
    
    // Underrun/overrun detection
    bool has_underrun();
    bool has_overrun();
    void reset_on_underrun();
    
    void cleanup();
};

#endif // AUDIO_RING_BUFFER_H
