/*
 * audio_capture_dummy.c - Dummy audio capture (silent)
 *
 * Always available fallback that generates silence.
 * Allows video-only streaming when audio hardware is unavailable.
 *
 * Parameters:
 * - 48000 Hz sample rate
 * - 2 channels (stereo)
 * - 240 samples per frame (5ms at 48kHz)
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 48000
#define CHANNELS 2
#define FRAME_SIZE 240  /* 5ms at 48kHz */

/*
 * Initialize dummy audio capture
 *
 * @param ctx RootStream context
 * @return    0 on success (always succeeds)
 */
int audio_capture_init_dummy(rootstream_ctx_t *ctx) {
    if (!ctx) {
        return -1;
    }

    printf("✓ Dummy audio capture ready (silent): %d Hz, %d channels, %d samples/frame\n",
           SAMPLE_RATE, CHANNELS, FRAME_SIZE);

    return 0;
}

/*
 * Capture one audio frame (returns silence)
 *
 * @param ctx         RootStream context
 * @param samples     Output PCM samples (interleaved stereo, 16-bit)
 * @param num_samples Output sample count
 * @return            0 on success (always succeeds)
 */
int audio_capture_frame_dummy(rootstream_ctx_t *ctx, int16_t *samples,
                              size_t *num_samples) {
    if (!ctx || !samples || !num_samples) {
        return -1;
    }

    /* Fill with silence (zeros) */
    memset(samples, 0, FRAME_SIZE * CHANNELS * sizeof(int16_t));
    *num_samples = FRAME_SIZE;

    return 0;
}

/*
 * Cleanup dummy audio capture
 */
void audio_capture_cleanup_dummy(rootstream_ctx_t *ctx) {
    if (!ctx) {
        return;
    }

    printf("✓ Dummy audio capture cleanup complete\n");
}
