/* PulseAudio Playback Backend for RootStream */
#ifndef PLAYBACK_PULSEAUDIO_H
#define PLAYBACK_PULSEAUDIO_H

#ifdef HAVE_PULSEAUDIO

#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <stdint.h>

class PulseAudioPlayback {
private:
    pa_simple *pa_simple;
    int sample_rate;
    int channels;
    bool playing;
    
public:
    PulseAudioPlayback();
    ~PulseAudioPlayback();
    
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
    
    // Volume control (simplified)
    int set_volume(float percent);
    float get_volume();
    
    void cleanup();
};

#endif // HAVE_PULSEAUDIO

#endif // PLAYBACK_PULSEAUDIO_H
