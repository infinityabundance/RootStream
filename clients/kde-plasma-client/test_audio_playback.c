/**
 * @file test_audio_playback.c
 * @brief Test program for audio playback subsystem
 * 
 * Tests audio backend initialization and basic playback.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

// Simple test - generate a sine wave tone

#define SAMPLE_RATE 48000
#define CHANNELS 2
#define DURATION_SEC 2
#define FREQUENCY 440.0f  // A4 note

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printf("RootStream Audio Playback Test\n");
    printf("===============================\n\n");
    
    printf("Configuration:\n");
    printf("  Sample Rate: %d Hz\n", SAMPLE_RATE);
    printf("  Channels: %d\n", CHANNELS);
    printf("  Duration: %d seconds\n", DURATION_SEC);
    printf("  Frequency: %.1f Hz\n\n", FREQUENCY);
    
    // Generate sine wave test tone
    int total_samples = SAMPLE_RATE * DURATION_SEC;
    float *audio_buffer = malloc(total_samples * CHANNELS * sizeof(float));
    
    if (!audio_buffer) {
        fprintf(stderr, "Failed to allocate audio buffer\n");
        return 1;
    }
    
    printf("Generating %d Hz sine wave...\n", (int)FREQUENCY);
    
    for (int i = 0; i < total_samples; i++) {
        float t = (float)i / (float)SAMPLE_RATE;
        float sample = 0.3f * sinf(2.0f * M_PI * FREQUENCY * t);
        
        // Apply fade in/out to avoid clicks
        float fade = 1.0f;
        if (i < SAMPLE_RATE / 10) {  // 100ms fade in
            fade = (float)i / (float)(SAMPLE_RATE / 10);
        } else if (i > total_samples - SAMPLE_RATE / 10) {  // 100ms fade out
            fade = (float)(total_samples - i) / (float)(SAMPLE_RATE / 10);
        }
        
        sample *= fade;
        
        // Stereo (same signal on both channels)
        audio_buffer[i * CHANNELS + 0] = sample;
        audio_buffer[i * CHANNELS + 1] = sample;
    }
    
    printf("✓ Generated %d samples\n\n", total_samples);
    
    // In a full test, we would initialize an audio backend and play
    printf("NOTE: Full audio playback requires Qt/C++ integration\n");
    printf("      This C test validates buffer generation only\n\n");
    
    printf("Test Plan:\n");
    printf("1. Initialize audio backend (PipeWire/PulseAudio/ALSA)\n");
    printf("2. Configure for %d Hz, %d channels\n", SAMPLE_RATE, CHANNELS);
    printf("3. Start playback\n");
    printf("4. Write sine wave samples\n");
    printf("5. Wait for playback completion\n");
    printf("6. Stop and cleanup\n\n");
    
    printf("Audio Statistics:\n");
    printf("  Buffer size: %zu bytes\n", total_samples * CHANNELS * sizeof(float));
    printf("  Duration: %.2f seconds\n", (float)total_samples / SAMPLE_RATE);
    printf("  Samples per channel: %d\n", total_samples);
    printf("  Total float samples: %d\n\n", total_samples * CHANNELS);
    
    // Calculate RMS level
    float rms = 0.0f;
    for (int i = 0; i < total_samples * CHANNELS; i++) {
        rms += audio_buffer[i] * audio_buffer[i];
    }
    rms = sqrtf(rms / (float)(total_samples * CHANNELS));
    printf("  RMS level: %.3f (%.1f dB)\n", rms, 20.0f * log10f(rms));
    
    free(audio_buffer);
    
    printf("\n✓ Audio buffer test complete\n");
    printf("\nFor full playback test, use the Qt-based AudioPlayer class\n");
    
    return 0;
}
