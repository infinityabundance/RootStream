/*
 * audio_playback_dummy.c - Dummy audio playback (discard)
 *
 * Always available fallback that discards audio.
 * Allows video-only viewing when audio hardware is unavailable.
 *
 * Parameters:
 * - 48000 Hz sample rate
 * - 2 channels (stereo)
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>

#define SAMPLE_RATE 48000
#define CHANNELS 2

/*
 * Initialize dummy audio playback
 *
 * @param ctx RootStream context
 * @return    0 on success (always succeeds)
 */
int audio_playback_init_dummy(rootstream_ctx_t *ctx) {
    if (!ctx) {
        return -1;
    }

    printf("✓ Dummy audio playback ready (silent): %d Hz, %d channels\n",
           SAMPLE_RATE, CHANNELS);

    return 0;
}

/*
 * Play audio samples (discards them)
 *
 * @param ctx         RootStream context
 * @param samples     PCM samples (interleaved stereo, 16-bit)
 * @param num_samples Sample count per channel
 * @return            0 on success (always succeeds)
 */
int audio_playback_write_dummy(rootstream_ctx_t *ctx, int16_t *samples,
                               size_t num_samples) {
    /* Do nothing - discard audio */
    (void)ctx;
    (void)samples;
    (void)num_samples;
    return 0;
}

/*
 * Cleanup dummy audio playback
 */
void audio_playback_cleanup_dummy(rootstream_ctx_t *ctx) {
    if (!ctx) {
        return;
    }

    printf("✓ Dummy audio playback cleanup complete\n");
}
