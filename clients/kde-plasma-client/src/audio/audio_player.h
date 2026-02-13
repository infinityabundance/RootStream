/* Audio Player Manager for RootStream */
#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <QObject>
#include <QThread>
#include <atomic>

// Forward declarations
class OpusDecoderWrapper;
class AudioRingBuffer;
class AudioResampler;
class AudioSync;
class PulseAudioPlayback;
class PipeWirePlayback;
class ALSAPlayback;

class AudioPlayer : public QObject {
    Q_OBJECT
    
private:
    OpusDecoderWrapper *opus_decoder;
    AudioRingBuffer *ring_buffer;
    AudioResampler *resampler;
    AudioSync *sync_manager;
    
    // Backend (only one will be used)
    void *playback_backend;
    int backend_type;
    
    QThread *decode_thread;
    QThread *playback_thread;
    std::atomic<bool> running;
    
    int sample_rate;
    int channels;
    int output_sample_rate;
    
    std::atomic<int> decoded_samples;
    std::atomic<int> dropped_packets;
    
public:
    explicit AudioPlayer(QObject *parent = nullptr);
    ~AudioPlayer();
    
    // Initialization
    int init(int sample_rate, int channels);
    
    // Network input
    int submit_audio_packet(const uint8_t *opus_packet, size_t packet_len,
                           uint64_t timestamp_us);
    
    // Playback control
    int start_playback();
    int stop_playback();
    int pause_playback();
    int resume_playback();
    
    // Configuration
    int set_output_device(const char *device);
    int set_volume(float percent);
    float get_volume();
    
    // State queries
    int get_latency_ms();
    int get_buffer_fill_percent();
    bool is_playing();
    
    // Statistics
    int get_decoded_samples() const { return decoded_samples.load(); }
    int get_dropped_packets() const { return dropped_packets.load(); }
    int get_audio_sync_offset_ms();
    
signals:
    void playback_started();
    void playback_stopped();
    void underrun_detected();
    void sync_warning(int offset_ms);
    void device_changed(const QString &device);
    
public slots:
    void on_video_frame_received(uint64_t timestamp_us);
    void on_network_latency_changed(uint32_t latency_ms);
    
private:
    void decode_thread_main();
    void playback_thread_main();
    
    void cleanup();
};

#endif // AUDIO_PLAYER_H
