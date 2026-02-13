/* ALSA Playback Backend for RootStream */
#ifndef PLAYBACK_ALSA_H
#define PLAYBACK_ALSA_H

#include <alsa/asoundlib.h>
#include <stdint.h>

class ALSAPlayback {
private:
    snd_pcm_t *pcm_handle;
    int sample_rate;
    int channels;
    snd_pcm_uframes_t period_size;
    bool playing;
    int underrun_count;
    
public:
    ALSAPlayback();
    ~ALSAPlayback();
    
    // Initialization
    int init(int sample_rate, int channels, const char *device = "default");
    
    // Playback control
    int start_playback();
    int stop_playback();
    int pause_playback();
    
    // Audio submission
    int write_samples(const float *samples, int sample_count);
    
    // State queries
    int get_buffer_latency_ms();
    bool is_playing() const { return playing; }
    int get_underrun_count() const { return underrun_count; }
    
    // Volume (mixer)
    int set_volume(float percent);
    float get_volume();
    
    void cleanup();
};

#endif // PLAYBACK_ALSA_H
