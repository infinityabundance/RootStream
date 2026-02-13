/* PulseAudio Playback Implementation */
#include "playback_pulseaudio.h"

#ifdef HAVE_PULSEAUDIO

#include <cstdio>
#include <cstring>

PulseAudioPlayback::PulseAudioPlayback()
    : pa_handle(nullptr), sample_rate(0), channels(0), playing(false) {
}

PulseAudioPlayback::~PulseAudioPlayback() {
    cleanup();
}

int PulseAudioPlayback::init(int sample_rate, int channels, const char *device) {
    if (pa_handle) {
        cleanup();
    }
    
    this->sample_rate = sample_rate;
    this->channels = channels;
    
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_FLOAT32LE;
    ss.rate = sample_rate;
    ss.channels = channels;
    
    pa_buffer_attr buffer_attr;
    memset(&buffer_attr, 0, sizeof(buffer_attr));
    buffer_attr.maxlength = (uint32_t)-1;
    buffer_attr.tlength = (sample_rate * sizeof(float) * channels * 50) / 1000; // 50ms buffer
    buffer_attr.prebuf = (uint32_t)-1;
    buffer_attr.minreq = (uint32_t)-1;
    buffer_attr.fragsize = (uint32_t)-1;
    
    int error;
    pa_handle = pa_simple_new(
        nullptr,                    // server
        "RootStream",               // application name
        PA_STREAM_PLAYBACK,         // direction
        device,                     // device (nullptr for default)
        "Game Audio",               // stream description
        &ss,                        // sample spec
        nullptr,                    // channel map
        &buffer_attr,               // buffer attributes
        &error                      // error code
    );
    
    if (!pa_handle) {
        fprintf(stderr, "Failed to create PulseAudio stream: %s\n",
                pa_strerror(error));
        return -1;
    }
    
    return 0;
}

int PulseAudioPlayback::start_playback() {
    playing = true;
    return 0;
}

int PulseAudioPlayback::stop_playback() {
    playing = false;
    if (pa_handle) {
        int error;
        pa_simple_drain(pa_handle, &error);
    }
    return 0;
}

int PulseAudioPlayback::pause_playback() {
    playing = false;
    return 0;
}

int PulseAudioPlayback::resume_playback() {
    playing = true;
    return 0;
}

int PulseAudioPlayback::write_samples(const float *samples, int sample_count) {
    if (!pa_handle || !playing) {
        return -1;
    }
    
    size_t bytes = sample_count * sizeof(float);
    int error;
    
    if (pa_simple_write(pa_handle, samples, bytes, &error) < 0) {
        fprintf(stderr, "PulseAudio write error: %s\n", pa_strerror(error));
        return -1;
    }
    
    return sample_count;
}

int PulseAudioPlayback::get_buffer_latency_ms() {
    if (!pa_handle) {
        return 0;
    }
    
    int error;
    pa_usec_t latency = pa_simple_get_latency(pa_handle, &error);
    
    if (latency == (pa_usec_t)-1) {
        return 0;
    }
    
    return (int)(latency / 1000);
}

int PulseAudioPlayback::set_volume(float percent) {
    // Volume control through PulseAudio context API is complex
    // For simplicity, we'll return success but not implement it
    (void)percent;
    return 0;
}

float PulseAudioPlayback::get_volume() {
    // Return default volume
    return 1.0f;
}

void PulseAudioPlayback::cleanup() {
    if (pa_handle) {
        int error;
        pa_simple_drain(pa_handle, &error);
        pa_simple_free(pa_handle);
        pa_handle = nullptr;
    }
    playing = false;
}

#endif // HAVE_PULSEAUDIO
