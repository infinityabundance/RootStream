/*
 * Unit tests for Audio components
 */

#include <QtTest/QtTest>
#include "../src/audio/opus_decoder.h"
#include "../src/audio/audio_ring_buffer.h"
#include "../src/audio/audio_resampler.h"
#include "../src/audio/audio_sync.h"
#include "../src/audio/audio_backend_selector.h"

class TestAudioComponents : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        // Setup
    }

    void testOpusDecoderInit() {
        OpusDecoderWrapper decoder;
        
        // Test initialization with 48kHz (standard Opus rate)
        QCOMPARE(decoder.init(48000, 2), 0);
        QCOMPARE(decoder.get_sample_rate(), 48000);
        QCOMPARE(decoder.get_channels(), 2);
        
        decoder.cleanup();
        
        // Test 16kHz (another standard Opus rate)
        QCOMPARE(decoder.init(16000, 2), 0);
        QCOMPARE(decoder.get_sample_rate(), 16000);
        
        decoder.cleanup();
    }

    void testRingBufferInit() {
        AudioRingBuffer buffer;
        
        // Test initialization
        QCOMPARE(buffer.init(48000, 2, 500), 0);
        
        // Check initial state
        QVERIFY(buffer.get_available_samples() == 0);
        QVERIFY(buffer.get_free_samples() > 0);
        QVERIFY(!buffer.has_underrun());
        QVERIFY(!buffer.has_overrun());
        
        buffer.cleanup();
    }

    void testRingBufferWriteRead() {
        AudioRingBuffer buffer;
        QCOMPARE(buffer.init(48000, 2, 100), 0);
        
        // Write some samples
        float input[100];
        for (int i = 0; i < 100; i++) {
            input[i] = (float)i / 100.0f;
        }
        
        QCOMPARE(buffer.write_samples(input, 100, 0), 100);
        QCOMPARE(buffer.get_available_samples(), 100);
        
        // Read them back
        float output[100];
        QCOMPARE(buffer.read_samples(output, 100, 0), 100);
        
        // Verify data
        for (int i = 0; i < 100; i++) {
            QCOMPARE(output[i], input[i]);
        }
        
        QCOMPARE(buffer.get_available_samples(), 0);
        
        buffer.cleanup();
    }

    void testResamplerInit() {
        AudioResampler resampler;
        
        // Test common resampling scenarios
        QCOMPARE(resampler.init(48000, 44100, 2), 0);
        QCOMPARE(resampler.get_input_rate(), 48000);
        QCOMPARE(resampler.get_output_rate(), 44100);
        QCOMPARE(resampler.get_channels(), 2);
        
        float expected_ratio = 44100.0f / 48000.0f;
        QVERIFY(qAbs(resampler.get_ratio() - expected_ratio) < 0.001f);
        
        resampler.cleanup();
    }

    void testAudioSyncInit() {
        AudioSync sync;
        
        QCOMPARE(sync.init(50), 0);
        
        // Initial state
        QCOMPARE(sync.get_current_av_offset_us(), (int64_t)0);
        QVERIFY(sync.is_in_sync());
        QCOMPARE(sync.get_sync_correction_count(), 0);
        
        sync.cleanup();
    }

    void testAudioSyncTimestamps() {
        AudioSync sync;
        QCOMPARE(sync.init(50), 0);
        
        // Set timestamps
        sync.update_video_timestamp(1000000);  // 1 second
        sync.update_audio_timestamp(1000000);  // 1 second
        
        int64_t offset = sync.calculate_sync_offset();
        QCOMPARE(offset, (int64_t)0);
        QVERIFY(sync.is_in_sync());
        
        // Audio ahead by 100ms
        sync.update_video_timestamp(1000000);
        sync.update_audio_timestamp(1100000);
        
        offset = sync.calculate_sync_offset();
        QCOMPARE(offset, (int64_t)-100000);
        QVERIFY(!sync.is_in_sync());  // > 50ms threshold
        
        sync.cleanup();
    }

    void testBackendSelector() {
        // Test backend detection
        AudioBackendSelector::AudioBackend backend = 
            AudioBackendSelector::detect_available_backend();
        
        // In a headless environment, no backend may be available
        // Just verify the function returns a valid enum value
        QVERIFY(backend >= AudioBackendSelector::AUDIO_BACKEND_NONE);
        QVERIFY(backend <= AudioBackendSelector::AUDIO_BACKEND_ALSA);
        
        // Get backend name
        const char *name = AudioBackendSelector::get_backend_name(backend);
        QVERIFY(name != nullptr);
        QVERIFY(strlen(name) > 0);
        
        // Test individual checks - at least one should work
        bool has_pulse = AudioBackendSelector::check_pulseaudio_available();
        bool has_pipewire = AudioBackendSelector::check_pipewire_available();
        bool has_alsa = AudioBackendSelector::check_alsa_available();
        
        // At least log what's available
        qDebug() << "PulseAudio:" << has_pulse;
        qDebug() << "PipeWire:" << has_pipewire;
        qDebug() << "ALSA:" << has_alsa;
    }

    void cleanupTestCase() {
        // Cleanup
    }
};

QTEST_MAIN(TestAudioComponents)
#include "test_audio_components.moc"
