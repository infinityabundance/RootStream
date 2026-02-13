/* Audio Backend Selector for RootStream */
#ifndef AUDIO_BACKEND_SELECTOR_H
#define AUDIO_BACKEND_SELECTOR_H

class AudioBackendSelector {
public:
    enum AudioBackend {
        AUDIO_BACKEND_NONE = 0,
        AUDIO_BACKEND_PULSEAUDIO,
        AUDIO_BACKEND_PIPEWIRE,
        AUDIO_BACKEND_ALSA,
    };
    
    // Detect available backend with fallback logic
    static AudioBackend detect_available_backend();
    
    // Check individual backends
    static bool check_pulseaudio_available();
    static bool check_pipewire_available();
    static bool check_alsa_available();
    
    // Get backend name as string
    static const char* get_backend_name(AudioBackend backend);
};

#endif // AUDIO_BACKEND_SELECTOR_H
