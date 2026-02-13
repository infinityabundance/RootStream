/*
 * audio_capture_pulse.c - PulseAudio capture fallback
 *
 * Fallback audio capture using PulseAudio Simple API.
 * More robust than ALSA on modern Linux distributions.
 *
 * Parameters:
 * - 48000 Hz sample rate
 * - 2 channels (stereo)
 * - 16-bit signed PCM
 * - 240 samples per frame (5ms at 48kHz)
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
#define FRAME_SIZE 240  /* 5ms at 48kHz */

typedef struct {
#ifdef HAVE_PULSEAUDIO
    pa_simple *stream;
#else
    void *dummy;
#endif
    int sample_rate;
    int channels;
    int frame_size;
    bool initialized;
} audio_capture_pulse_ctx_t;

/*
 * Check if PulseAudio is available
 */
bool audio_capture_pulse_available(void) {
#ifdef HAVE_PULSEAUDIO
    /* Try to create a test connection */
    int error;
    pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = SAMPLE_RATE,
        .channels = CHANNELS
    };
    
    pa_simple *test = pa_simple_new(NULL, "RootStream-Test", PA_STREAM_RECORD,
                                    NULL, "test", &ss, NULL, NULL, &error);
    if (test) {
        pa_simple_free(test);
        return true;
    }
#endif
    return false;
}

/*
 * Initialize PulseAudio audio capture
 *
 * @param ctx RootStream context
 * @return    0 on success, -1 on error
 */
int audio_capture_init_pulse(rootstream_ctx_t *ctx) {
#ifdef HAVE_PULSEAUDIO
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context for PulseAudio capture\n");
        return -1;
    }

    /* Allocate capture context */
    audio_capture_pulse_ctx_t *capture = calloc(1, sizeof(audio_capture_pulse_ctx_t));
    if (!capture) {
        fprintf(stderr, "ERROR: Cannot allocate PulseAudio capture context\n");
        return -1;
    }

    capture->sample_rate = SAMPLE_RATE;
    capture->channels = CHANNELS;
    capture->frame_size = FRAME_SIZE;

    /* Configure sample format */
    pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = capture->sample_rate,
        .channels = capture->channels
    };

    /* Configure buffer attributes for low latency */
    pa_buffer_attr attr = {
        .maxlength = (uint32_t)-1,
        .tlength = (uint32_t)-1,
        .prebuf = (uint32_t)-1,
        .minreq = (uint32_t)-1,
        .fragsize = capture->frame_size * sizeof(int16_t) * capture->channels
    };

    /* Create PulseAudio stream */
    int error;
    capture->stream = pa_simple_new(
        NULL,                       /* Use default server */
        "RootStream",               /* Application name */
        PA_STREAM_RECORD,           /* Recording mode */
        NULL,                       /* Use default device */
        "Audio Capture",            /* Stream description */
        &ss,                        /* Sample format */
        NULL,                       /* Use default channel map */
        &attr,                      /* Buffer attributes */
        &error                      /* Error code */
    );

    if (!capture->stream) {
        fprintf(stderr, "ERROR: Cannot open PulseAudio stream: %s\n",
                pa_strerror(error));
        free(capture);
        return -1;
    }

    capture->initialized = true;

    /* Store in context (reuse mouse fd field) */
    ctx->uinput_mouse_fd = (int)(intptr_t)capture;

    printf("✓ PulseAudio capture ready: %d Hz, %d channels, %d samples/frame\n",
           capture->sample_rate, capture->channels, capture->frame_size);

    return 0;
#else
    fprintf(stderr, "ERROR: PulseAudio support not compiled\n");
    return -1;
#endif
}

/*
 * Capture one audio frame
 *
 * @param ctx         RootStream context
 * @param samples     Output PCM samples (interleaved stereo, 16-bit)
 * @param num_samples Output sample count
 * @return            0 on success, -1 on error
 */
int audio_capture_frame_pulse(rootstream_ctx_t *ctx, int16_t *samples,
                              size_t *num_samples) {
#ifdef HAVE_PULSEAUDIO
    if (!ctx || !samples || !num_samples) {
        return -1;
    }

    audio_capture_pulse_ctx_t *capture = (audio_capture_pulse_ctx_t*)(intptr_t)ctx->uinput_mouse_fd;
    if (!capture || !capture->initialized || !capture->stream) {
        return -1;
    }

    /* Read PCM samples from PulseAudio */
    size_t bytes_to_read = capture->frame_size * sizeof(int16_t) * capture->channels;
    int error;

    if (pa_simple_read(capture->stream, samples, bytes_to_read, &error) < 0) {
        fprintf(stderr, "ERROR: PulseAudio read failed: %s\n",
                pa_strerror(error));
        return -1;
    }

    *num_samples = capture->frame_size;
    return 0;
#else
    (void)ctx;
    (void)samples;
    (void)num_samples;
    return -1;
#endif
}

/*
 * Cleanup PulseAudio capture
 */
void audio_capture_cleanup_pulse(rootstream_ctx_t *ctx) {
#ifdef HAVE_PULSEAUDIO
    if (!ctx || !ctx->uinput_mouse_fd) {
        return;
    }

    audio_capture_pulse_ctx_t *capture = (audio_capture_pulse_ctx_t*)(intptr_t)ctx->uinput_mouse_fd;

    if (capture->stream) {
        pa_simple_free(capture->stream);
    }

    free(capture);
    ctx->uinput_mouse_fd = 0;

    printf("✓ PulseAudio capture cleanup complete\n");
#else
    (void)ctx;
#endif
}
