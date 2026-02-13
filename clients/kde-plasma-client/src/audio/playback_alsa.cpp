/* ALSA Playback Implementation */
#include "playback_alsa.h"
#include <cstdio>
#include <cstring>

ALSAPlayback::ALSAPlayback()
    : pcm_handle(nullptr), sample_rate(0), channels(0), 
      period_size(0), playing(false), underrun_count(0) {
}

ALSAPlayback::~ALSAPlayback() {
    cleanup();
}

int ALSAPlayback::init(int sample_rate, int channels, const char *device) {
    if (pcm_handle) {
        cleanup();
    }
    
    this->sample_rate = sample_rate;
    this->channels = channels;
    
    int err = snd_pcm_open(&pcm_handle, device, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        fprintf(stderr, "Failed to open ALSA device %s: %s\n",
                device, snd_strerror(err));
        return -1;
    }
    
    // Set hardware parameters
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);
    
    err = snd_pcm_hw_params_any(pcm_handle, hw_params);
    if (err < 0) {
        fprintf(stderr, "Failed to initialize hw params: %s\n", snd_strerror(err));
        cleanup();
        return -1;
    }
    
    err = snd_pcm_hw_params_set_access(pcm_handle, hw_params, 
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        fprintf(stderr, "Failed to set access type: %s\n", snd_strerror(err));
        cleanup();
        return -1;
    }
    
    err = snd_pcm_hw_params_set_format(pcm_handle, hw_params, 
                                       SND_PCM_FORMAT_FLOAT_LE);
    if (err < 0) {
        fprintf(stderr, "Failed to set sample format: %s\n", snd_strerror(err));
        cleanup();
        return -1;
    }
    
    unsigned int rate = sample_rate;
    err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &rate, 0);
    if (err < 0) {
        fprintf(stderr, "Failed to set sample rate: %s\n", snd_strerror(err));
        cleanup();
        return -1;
    }
    
    err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, channels);
    if (err < 0) {
        fprintf(stderr, "Failed to set channels: %s\n", snd_strerror(err));
        cleanup();
        return -1;
    }
    
    // Set buffer time to 100ms
    unsigned int buffer_time = 100000;
    err = snd_pcm_hw_params_set_buffer_time_near(pcm_handle, hw_params, 
                                                  &buffer_time, 0);
    if (err < 0) {
        fprintf(stderr, "Failed to set buffer time: %s\n", snd_strerror(err));
        cleanup();
        return -1;
    }
    
    // Set period time to 25ms
    unsigned int period_time = 25000;
    err = snd_pcm_hw_params_set_period_time_near(pcm_handle, hw_params,
                                                  &period_time, 0);
    if (err < 0) {
        fprintf(stderr, "Failed to set period time: %s\n", snd_strerror(err));
        cleanup();
        return -1;
    }
    
    err = snd_pcm_hw_params(pcm_handle, hw_params);
    if (err < 0) {
        fprintf(stderr, "Failed to set hw params: %s\n", snd_strerror(err));
        cleanup();
        return -1;
    }
    
    snd_pcm_hw_params_get_period_size(hw_params, &period_size, 0);
    
    // Prepare device
    err = snd_pcm_prepare(pcm_handle);
    if (err < 0) {
        fprintf(stderr, "Failed to prepare ALSA device: %s\n", snd_strerror(err));
        cleanup();
        return -1;
    }
    
    return 0;
}

int ALSAPlayback::start_playback() {
    playing = true;
    return 0;
}

int ALSAPlayback::stop_playback() {
    playing = false;
    if (pcm_handle) {
        snd_pcm_drain(pcm_handle);
    }
    return 0;
}

int ALSAPlayback::pause_playback() {
    playing = false;
    if (pcm_handle) {
        snd_pcm_pause(pcm_handle, 1);
    }
    return 0;
}

int ALSAPlayback::write_samples(const float *samples, int sample_count) {
    if (!pcm_handle || !playing) {
        return -1;
    }
    
    int frames = sample_count / channels;
    snd_pcm_sframes_t written = snd_pcm_writei(pcm_handle, samples, frames);
    
    if (written < 0) {
        if (written == -EPIPE) {
            // Underrun occurred
            underrun_count++;
            fprintf(stderr, "ALSA underrun occurred\n");
            snd_pcm_prepare(pcm_handle);
            written = snd_pcm_writei(pcm_handle, samples, frames);
        } else {
            fprintf(stderr, "ALSA write error: %s\n", snd_strerror(written));
            return -1;
        }
    }
    
    return written * channels;
}

int ALSAPlayback::get_buffer_latency_ms() {
    if (!pcm_handle) {
        return 0;
    }
    
    snd_pcm_sframes_t delay;
    int err = snd_pcm_delay(pcm_handle, &delay);
    if (err < 0) {
        return 0;
    }
    
    return (int)((delay * 1000) / sample_rate);
}

int ALSAPlayback::set_volume(float percent) {
    // Volume control through ALSA mixer is complex
    // For simplicity, we'll return success but not implement it
    (void)percent;
    return 0;
}

float ALSAPlayback::get_volume() {
    return 1.0f;
}

void ALSAPlayback::cleanup() {
    if (pcm_handle) {
        snd_pcm_close(pcm_handle);
        pcm_handle = nullptr;
    }
    playing = false;
}
