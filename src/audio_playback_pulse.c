/*
 * audio_playback_pulse.c - PulseAudio playback fallback
 *
 * Fallback audio playback using PulseAudio Simple API.
 * More robust than ALSA on modern Linux distributions.
 *
 * Parameters:
 * - 48000 Hz sample rate
 * - 2 channels (stereo)
 * - 16-bit signed PCM
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_PULSEAUDIO
#include <pulse/simple.h>
#include <pulse/error.h>
#endif

#define SAMPLE_RATE 48000
#define CHANNELS 2

typedef struct {
#ifdef HAVE_PULSEAUDIO
    pa_simple *stream;
#else
    void *dummy;
#endif
    int sample_rate;
    int channels;
    bool initialized;
} audio_playback_pulse_ctx_t;

/*
 * Check if PulseAudio is available
 */
bool audio_playback_pulse_available(void) {
#ifdef HAVE_PULSEAUDIO
    /* Try to create a test connection */
    int error;
    pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = SAMPLE_RATE,
        .channels = CHANNELS
    };
    
    pa_simple *test = pa_simple_new(NULL, "RootStream-Test", PA_STREAM_PLAYBACK,
                                    NULL, "test", &ss, NULL, NULL, &error);
    if (test) {
        pa_simple_free(test);
        return true;
    }
#endif
    return false;
}

/*
 * Initialize PulseAudio audio playback
 *
 * @param ctx RootStream context
 * @return    0 on success, -1 on error
 */
int audio_playback_init_pulse(rootstream_ctx_t *ctx) {
#ifdef HAVE_PULSEAUDIO
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context for PulseAudio playback\n");
        return -1;
    }

    /* Allocate playback context */
    audio_playback_pulse_ctx_t *playback = calloc(1, sizeof(audio_playback_pulse_ctx_t));
    if (!playback) {
        fprintf(stderr, "ERROR: Cannot allocate PulseAudio playback context\n");
        return -1;
    }

    playback->sample_rate = SAMPLE_RATE;
    playback->channels = CHANNELS;

    /* Configure sample format */
    pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = playback->sample_rate,
        .channels = playback->channels
    };

    /* Configure buffer attributes for low latency */
    pa_buffer_attr attr = {
        .maxlength = (uint32_t)-1,
        .tlength = 240 * sizeof(int16_t) * playback->channels * 4,  /* 20ms buffer */
        .prebuf = (uint32_t)-1,
        .minreq = (uint32_t)-1,
        .fragsize = (uint32_t)-1
    };

    /* Create PulseAudio stream */
    int error;
    playback->stream = pa_simple_new(
        NULL,                       /* Use default server */
        "RootStream",               /* Application name */
        PA_STREAM_PLAYBACK,         /* Playback mode */
        NULL,                       /* Use default device */
        "Audio Playback",           /* Stream description */
        &ss,                        /* Sample format */
        NULL,                       /* Use default channel map */
        &attr,                      /* Buffer attributes */
        &error                      /* Error code */
    );

    if (!playback->stream) {
        fprintf(stderr, "ERROR: Cannot open PulseAudio stream: %s\n",
                pa_strerror(error));
        free(playback);
        return -1;
    }

    playback->initialized = true;

    /* Store in context (reuse tray menu field) */
    ctx->tray.menu = playback;

    printf("✓ PulseAudio playback ready: %d Hz, %d channels\n",
           playback->sample_rate, playback->channels);

    return 0;
#else
    fprintf(stderr, "ERROR: PulseAudio support not compiled\n");
    return -1;
#endif
}

/*
 * Play audio samples
 *
 * @param ctx         RootStream context
 * @param samples     PCM samples (interleaved stereo, 16-bit)
 * @param num_samples Sample count per channel
 * @return            0 on success, -1 on error
 */
int audio_playback_write_pulse(rootstream_ctx_t *ctx, int16_t *samples,
                               size_t num_samples) {
#ifdef HAVE_PULSEAUDIO
    if (!ctx || !samples || num_samples == 0) {
        return -1;
    }

    audio_playback_pulse_ctx_t *playback = (audio_playback_pulse_ctx_t*)ctx->tray.menu;
    if (!playback || !playback->initialized || !playback->stream) {
        return -1;
    }

    /* Write PCM samples to PulseAudio */
    size_t bytes_to_write = num_samples * sizeof(int16_t) * playback->channels;
    int error;

    if (pa_simple_write(playback->stream, samples, bytes_to_write, &error) < 0) {
        fprintf(stderr, "ERROR: PulseAudio write failed: %s\n",
                pa_strerror(error));
        return -1;
    }

    return 0;
#else
    (void)ctx;
    (void)samples;
    (void)num_samples;
    return -1;
#endif
}

/*
 * Cleanup PulseAudio playback
 */
void audio_playback_cleanup_pulse(rootstream_ctx_t *ctx) {
#ifdef HAVE_PULSEAUDIO
    if (!ctx || !ctx->tray.menu) {
        return;
    }

    audio_playback_pulse_ctx_t *playback = (audio_playback_pulse_ctx_t*)ctx->tray.menu;

    if (playback->stream) {
        /* Drain any remaining audio */
        int error;
        pa_simple_drain(playback->stream, &error);
        pa_simple_free(playback->stream);
    }

    free(playback);
    ctx->tray.menu = NULL;

    printf("✓ PulseAudio playback cleanup complete\n");
#else
    (void)ctx;
#endif
}
