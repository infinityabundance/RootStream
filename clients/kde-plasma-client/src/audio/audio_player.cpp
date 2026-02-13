/* Audio Player Implementation */
#include "audio_player.h"
#include "opus_decoder.h"
#include "audio_ring_buffer.h"
#include "audio_resampler.h"
#include "audio_sync.h"
#include "audio_backend_selector.h"
#include "playback_pulseaudio.h"
#include "playback_pipewire.h"
#include "playback_alsa.h"

#include <cstdio>
#include <cstring>
#include <unistd.h>

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent), opus_decoder(nullptr), ring_buffer(nullptr),
      resampler(nullptr), sync_manager(nullptr), playback_backend(nullptr),
      backend_type(0), decode_thread(nullptr), playback_thread(nullptr),
      running(false), sample_rate(0), channels(0), output_sample_rate(0),
      decoded_samples(0), dropped_packets(0) {
}

AudioPlayer::~AudioPlayer() {
    cleanup();
}

int AudioPlayer::init(int sample_rate, int channels) {
    this->sample_rate = sample_rate;
    this->channels = channels;
    this->output_sample_rate = 48000; // Default output rate
    
    // Initialize Opus decoder
    opus_decoder = new OpusDecoderWrapper();
    if (opus_decoder->init(sample_rate, channels) < 0) {
        fprintf(stderr, "Failed to initialize Opus decoder\n");
        cleanup();
        return -1;
    }
    
    // Initialize ring buffer (500ms buffer)
    ring_buffer = new AudioRingBuffer();
    if (ring_buffer->init(sample_rate, channels, 500) < 0) {
        fprintf(stderr, "Failed to initialize ring buffer\n");
        cleanup();
        return -1;
    }
    
    // Initialize resampler if needed
    if (sample_rate != output_sample_rate) {
        resampler = new AudioResampler();
        if (resampler->init(sample_rate, output_sample_rate, channels) < 0) {
            fprintf(stderr, "Failed to initialize resampler\n");
            cleanup();
            return -1;
        }
    }
    
    // Initialize sync manager
    sync_manager = new AudioSync();
    if (sync_manager->init() < 0) {
        fprintf(stderr, "Failed to initialize sync manager\n");
        cleanup();
        return -1;
    }
    
    // Detect and initialize audio backend
    AudioBackendSelector::AudioBackend backend = 
        AudioBackendSelector::detect_available_backend();
    
    fprintf(stderr, "Audio backend: %s\n", 
            AudioBackendSelector::get_backend_name(backend));
    
    switch (backend) {
#ifdef HAVE_PULSEAUDIO
        case AudioBackendSelector::AUDIO_BACKEND_PULSEAUDIO: {
            PulseAudioPlayback *pa = new PulseAudioPlayback();
            if (pa->init(output_sample_rate, channels) < 0) {
                delete pa;
                fprintf(stderr, "Failed to initialize PulseAudio, trying fallback\n");
                backend = AudioBackendSelector::AUDIO_BACKEND_ALSA;
                goto try_alsa;
            }
            playback_backend = pa;
            backend_type = backend;
            break;
        }
#endif
        
#ifdef HAVE_PIPEWIRE
        case AudioBackendSelector::AUDIO_BACKEND_PIPEWIRE: {
            PipeWirePlayback *pw = new PipeWirePlayback();
            if (pw->init(output_sample_rate, channels) < 0) {
                delete pw;
                fprintf(stderr, "Failed to initialize PipeWire, trying fallback\n");
                backend = AudioBackendSelector::AUDIO_BACKEND_ALSA;
                goto try_alsa;
            }
            playback_backend = pw;
            backend_type = backend;
            break;
        }
#endif
        
        case AudioBackendSelector::AUDIO_BACKEND_ALSA:
        default:
try_alsa:
        {
            ALSAPlayback *alsa = new ALSAPlayback();
            if (alsa->init(output_sample_rate, channels) < 0) {
                delete alsa;
                fprintf(stderr, "Failed to initialize ALSA\n");
                cleanup();
                return -1;
            }
            playback_backend = alsa;
            backend_type = AudioBackendSelector::AUDIO_BACKEND_ALSA;
            break;
        }
    }
    
    return 0;
}

int AudioPlayer::submit_audio_packet(const uint8_t *opus_packet, 
                                     size_t packet_len,
                                     uint64_t timestamp_us) {
    if (!opus_decoder || !ring_buffer) {
        return -1;
    }
    
    // Decode Opus packet
    const int max_samples = 5760; // Max Opus frame size
    float pcm_buffer[max_samples * 2]; // stereo
    
    int samples = opus_decoder->decode_frame(opus_packet, packet_len,
                                             pcm_buffer, max_samples);
    
    if (samples < 0) {
        dropped_packets++;
        return -1;
    }
    
    decoded_samples += samples;
    
    // Update audio timestamp
    if (sync_manager) {
        sync_manager->update_audio_timestamp(timestamp_us);
    }
    
    // Write to ring buffer
    int total_samples = samples * channels;
    int written = ring_buffer->write_samples(pcm_buffer, total_samples, 100);
    
    if (written < 0) {
        fprintf(stderr, "Ring buffer overflow\n");
        return -1;
    }
    
    return 0;
}

int AudioPlayer::start_playback() {
    if (!playback_backend) {
        return -1;
    }
    
    running = true;
    
    // Start playback based on backend type
    switch (backend_type) {
#ifdef HAVE_PULSEAUDIO
        case AudioBackendSelector::AUDIO_BACKEND_PULSEAUDIO:
            return ((PulseAudioPlayback*)playback_backend)->start_playback();
#endif
#ifdef HAVE_PIPEWIRE
        case AudioBackendSelector::AUDIO_BACKEND_PIPEWIRE:
            return ((PipeWirePlayback*)playback_backend)->start_playback();
#endif
        case AudioBackendSelector::AUDIO_BACKEND_ALSA:
            return ((ALSAPlayback*)playback_backend)->start_playback();
        default:
            return -1;
    }
}

int AudioPlayer::stop_playback() {
    running = false;
    
    if (!playback_backend) {
        return -1;
    }
    
    // Stop playback based on backend type
    switch (backend_type) {
#ifdef HAVE_PULSEAUDIO
        case AudioBackendSelector::AUDIO_BACKEND_PULSEAUDIO:
            return ((PulseAudioPlayback*)playback_backend)->stop_playback();
#endif
#ifdef HAVE_PIPEWIRE
        case AudioBackendSelector::AUDIO_BACKEND_PIPEWIRE:
            return ((PipeWirePlayback*)playback_backend)->stop_playback();
#endif
        case AudioBackendSelector::AUDIO_BACKEND_ALSA:
            return ((ALSAPlayback*)playback_backend)->stop_playback();
        default:
            return -1;
    }
}

int AudioPlayer::pause_playback() {
    if (!playback_backend) {
        return -1;
    }
    
    // Pause based on backend type
    switch (backend_type) {
#ifdef HAVE_PULSEAUDIO
        case AudioBackendSelector::AUDIO_BACKEND_PULSEAUDIO:
            return ((PulseAudioPlayback*)playback_backend)->pause_playback();
#endif
#ifdef HAVE_PIPEWIRE
        case AudioBackendSelector::AUDIO_BACKEND_PIPEWIRE:
            return ((PipeWirePlayback*)playback_backend)->pause_playback();
#endif
        case AudioBackendSelector::AUDIO_BACKEND_ALSA:
            return ((ALSAPlayback*)playback_backend)->pause_playback();
        default:
            return -1;
    }
}

int AudioPlayer::resume_playback() {
    if (!playback_backend) {
        return -1;
    }
    
    // Resume based on backend type
    switch (backend_type) {
#ifdef HAVE_PULSEAUDIO
        case AudioBackendSelector::AUDIO_BACKEND_PULSEAUDIO:
            return ((PulseAudioPlayback*)playback_backend)->resume_playback();
#endif
#ifdef HAVE_PIPEWIRE
        case AudioBackendSelector::AUDIO_BACKEND_PIPEWIRE:
            return ((PipeWirePlayback*)playback_backend)->resume_playback();
#endif
        case AudioBackendSelector::AUDIO_BACKEND_ALSA:
            return ((ALSAPlayback*)playback_backend)->start_playback();
        default:
            return -1;
    }
}

int AudioPlayer::set_output_device(const char *device) {
    // Not implemented - would require re-initialization
    (void)device;
    return -1;
}

int AudioPlayer::set_volume(float percent) {
    if (!playback_backend) {
        return -1;
    }
    
    // Set volume based on backend type
    switch (backend_type) {
#ifdef HAVE_PULSEAUDIO
        case AudioBackendSelector::AUDIO_BACKEND_PULSEAUDIO:
            return ((PulseAudioPlayback*)playback_backend)->set_volume(percent);
#endif
#ifdef HAVE_PIPEWIRE
        case AudioBackendSelector::AUDIO_BACKEND_PIPEWIRE:
            return ((PipeWirePlayback*)playback_backend)->set_volume(percent);
#endif
        case AudioBackendSelector::AUDIO_BACKEND_ALSA:
            return ((ALSAPlayback*)playback_backend)->set_volume(percent);
        default:
            return -1;
    }
}

float AudioPlayer::get_volume() {
    if (!playback_backend) {
        return 1.0f;
    }
    
    // Get volume based on backend type
    switch (backend_type) {
#ifdef HAVE_PULSEAUDIO
        case AudioBackendSelector::AUDIO_BACKEND_PULSEAUDIO:
            return ((PulseAudioPlayback*)playback_backend)->get_volume();
#endif
#ifdef HAVE_PIPEWIRE
        case AudioBackendSelector::AUDIO_BACKEND_PIPEWIRE:
            return ((PipeWirePlayback*)playback_backend)->get_volume();
#endif
        case AudioBackendSelector::AUDIO_BACKEND_ALSA:
            return ((ALSAPlayback*)playback_backend)->get_volume();
        default:
            return 1.0f;
    }
}

int AudioPlayer::get_latency_ms() {
    if (!ring_buffer) {
        return 0;
    }
    return ring_buffer->get_latency_ms();
}

int AudioPlayer::get_buffer_fill_percent() {
    if (!ring_buffer) {
        return 0;
    }
    return (int)ring_buffer->get_fill_percentage();
}

bool AudioPlayer::is_playing() {
    if (!playback_backend) {
        return false;
    }
    
    // Check if playing based on backend type
    switch (backend_type) {
#ifdef HAVE_PULSEAUDIO
        case AudioBackendSelector::AUDIO_BACKEND_PULSEAUDIO:
            return ((PulseAudioPlayback*)playback_backend)->is_playing();
#endif
#ifdef HAVE_PIPEWIRE
        case AudioBackendSelector::AUDIO_BACKEND_PIPEWIRE:
            return ((PipeWirePlayback*)playback_backend)->is_playing();
#endif
        case AudioBackendSelector::AUDIO_BACKEND_ALSA:
            return ((ALSAPlayback*)playback_backend)->is_playing();
        default:
            return false;
    }
}

int AudioPlayer::get_audio_sync_offset_ms() {
    if (!sync_manager) {
        return 0;
    }
    return (int)(sync_manager->get_current_av_offset_us() / 1000);
}

void AudioPlayer::on_video_frame_received(uint64_t timestamp_us) {
    if (sync_manager) {
        sync_manager->update_video_timestamp(timestamp_us);
        
        // Check sync and emit warning if needed
        int64_t offset_us = sync_manager->calculate_sync_offset();
        int offset_ms = (int)(offset_us / 1000);
        
        if (abs(offset_ms) > 100) {
            emit sync_warning(offset_ms);
        }
    }
}

void AudioPlayer::on_network_latency_changed(uint32_t latency_ms) {
    // Could adjust buffer size based on network latency
    (void)latency_ms;
}

void AudioPlayer::cleanup() {
    running = false;
    
    // Cleanup threads
    if (decode_thread) {
        decode_thread->quit();
        decode_thread->wait();
        delete decode_thread;
        decode_thread = nullptr;
    }
    
    if (playback_thread) {
        playback_thread->quit();
        playback_thread->wait();
        delete playback_thread;
        playback_thread = nullptr;
    }
    
    // Cleanup playback backend
    if (playback_backend) {
        switch (backend_type) {
#ifdef HAVE_PULSEAUDIO
            case AudioBackendSelector::AUDIO_BACKEND_PULSEAUDIO:
                delete (PulseAudioPlayback*)playback_backend;
                break;
#endif
#ifdef HAVE_PIPEWIRE
            case AudioBackendSelector::AUDIO_BACKEND_PIPEWIRE:
                delete (PipeWirePlayback*)playback_backend;
                break;
#endif
            case AudioBackendSelector::AUDIO_BACKEND_ALSA:
                delete (ALSAPlayback*)playback_backend;
                break;
        }
        playback_backend = nullptr;
    }
    
    // Cleanup audio components
    if (opus_decoder) {
        opus_decoder->cleanup();
        delete opus_decoder;
        opus_decoder = nullptr;
    }
    
    if (ring_buffer) {
        ring_buffer->cleanup();
        delete ring_buffer;
        ring_buffer = nullptr;
    }
    
    if (resampler) {
        resampler->cleanup();
        delete resampler;
        resampler = nullptr;
    }
    
    if (sync_manager) {
        sync_manager->cleanup();
        delete sync_manager;
        sync_manager = nullptr;
    }
}
