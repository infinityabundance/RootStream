/* Audio Backend Selector Implementation */
#include "audio_backend_selector.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#ifdef HAVE_PULSEAUDIO
#include <pulse/simple.h>
#endif

AudioBackendSelector::AudioBackend AudioBackendSelector::detect_available_backend() {
    // Priority order:
    // 1. Try PulseAudio
    if (check_pulseaudio_available()) {
        return AUDIO_BACKEND_PULSEAUDIO;
    }
    
    // 2. Try PipeWire
    if (check_pipewire_available()) {
        return AUDIO_BACKEND_PIPEWIRE;
    }
    
    // 3. Fall back to ALSA
    if (check_alsa_available()) {
        return AUDIO_BACKEND_ALSA;
    }
    
    return AUDIO_BACKEND_NONE;
}

bool AudioBackendSelector::check_pulseaudio_available() {
#ifdef HAVE_PULSEAUDIO
    // Try to create a simple PulseAudio connection
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_FLOAT32LE;
    ss.rate = 48000;
    ss.channels = 2;
    
    pa_simple *test = pa_simple_new(
        nullptr,            // server
        "RootStream-Test",  // app name
        PA_STREAM_PLAYBACK, // direction
        nullptr,            // device
        "test",             // stream name
        &ss,                // sample spec
        nullptr,            // channel map
        nullptr,            // buffer attributes
        nullptr             // error code
    );
    
    if (test) {
        pa_simple_free(test);
        return true;
    }
#endif
    
    return false;
}

bool AudioBackendSelector::check_pipewire_available() {
#ifdef HAVE_PIPEWIRE
    // Check for PipeWire runtime directory
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir) {
        char path[512];
        snprintf(path, sizeof(path), "%s/pipewire-0", runtime_dir);
        if (access(path, F_OK) == 0) {
            return true;
        }
    }
#endif
    
    return false;
}

bool AudioBackendSelector::check_alsa_available() {
    // ALSA is always available on Linux systems
    // Check if ALSA device exists
    if (access("/dev/snd", F_OK) == 0) {
        return true;
    }
    
    return false;
}

const char* AudioBackendSelector::get_backend_name(AudioBackend backend) {
    switch (backend) {
        case AUDIO_BACKEND_PULSEAUDIO:
            return "PulseAudio";
        case AUDIO_BACKEND_PIPEWIRE:
            return "PipeWire";
        case AUDIO_BACKEND_ALSA:
            return "ALSA";
        case AUDIO_BACKEND_NONE:
        default:
            return "None";
    }
}
