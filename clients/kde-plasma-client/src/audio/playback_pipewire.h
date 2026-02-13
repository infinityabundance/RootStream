/* PipeWire Playback Backend for RootStream */
#ifndef PLAYBACK_PIPEWIRE_H
#define PLAYBACK_PIPEWIRE_H

#ifdef HAVE_PIPEWIRE

#include <stdint.h>

// Forward declarations (PipeWire headers are complex)
struct pw_thread_loop;
struct pw_stream;

class PipeWirePlayback {
private:
    pw_thread_loop *loop;
    pw_stream *stream;
    int sample_rate;
    int channels;
    bool playing;
    
public:
    PipeWirePlayback();
    ~PipeWirePlayback();
    
    // Initialization
    int init(int sample_rate, int channels, const char *device = nullptr);
    
    // Playback control
    int start_playback();
    int stop_playback();
    int pause_playback();
    int resume_playback();
    
    // Audio submission
    int write_samples(const float *samples, int sample_count);
    
    // State queries
    int get_buffer_latency_ms();
    bool is_playing() const { return playing; }
    
    // Volume control
    int set_volume(float percent);
    float get_volume();
    
    void cleanup();
};

#endif // HAVE_PIPEWIRE

#endif // PLAYBACK_PIPEWIRE_H
