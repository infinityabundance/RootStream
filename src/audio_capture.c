/*
 * audio_capture.c - ALSA audio capture for host
 *
 * Captures system audio using ALSA (Advanced Linux Sound Architecture).
 * Configured for low-latency capture to match video streaming.
 *
 * Parameters:
 * - 48000 Hz sample rate (Opus native)
 * - 2 channels (stereo)
 * - 16-bit signed PCM
 * - 5ms frames (240 samples at 48kHz)
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <alsa/asoundlib.h>

#define SAMPLE_RATE 48000
#define CHANNELS 2
#define FRAME_SIZE 240  /* 5ms at 48kHz */

typedef struct {
    snd_pcm_t *handle;
    int sample_rate;
    int channels;
    int frame_size;
    bool initialized;
} alsa_capture_ctx_t;

/*
 * Check if ALSA is available
 */
bool audio_capture_alsa_available(void) {
    /* Try to open and immediately close a test handle */
    snd_pcm_t *handle = NULL;
    int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (err >= 0 && handle) {
        snd_pcm_close(handle);
        return true;
    }
    return false;
}

/*
 * Initialize ALSA audio capture
 *
 * @param ctx RootStream context
 * @return    0 on success, -1 on error
 */
int audio_capture_init_alsa(rootstream_ctx_t *ctx) {
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context for audio capture\n");
        return -1;
    }

    /* Allocate capture context */
    alsa_capture_ctx_t *capture = calloc(1, sizeof(alsa_capture_ctx_t));
    if (!capture) {
        fprintf(stderr, "ERROR: Cannot allocate audio capture context\n");
        return -1;
    }

    capture->sample_rate = SAMPLE_RATE;
    capture->channels = CHANNELS;
    capture->frame_size = FRAME_SIZE;

    /* Open ALSA device for capture */
    int err = snd_pcm_open(&capture->handle, "default",
                          SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
        fprintf(stderr, "ERROR: Cannot open audio capture device: %s\n",
                snd_strerror(err));
        free(capture);
        return -1;
    }

    /* Configure hardware parameters */
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(capture->handle, hw_params);

    /* Set access type (interleaved) */
    err = snd_pcm_hw_params_set_access(capture->handle, hw_params,
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        fprintf(stderr, "ERROR: Cannot set audio access type: %s\n",
                snd_strerror(err));
        snd_pcm_close(capture->handle);
        free(capture);
        return -1;
    }

    /* Set sample format (16-bit signed) */
    err = snd_pcm_hw_params_set_format(capture->handle, hw_params,
                                       SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
        fprintf(stderr, "ERROR: Cannot set audio format: %s\n",
                snd_strerror(err));
        snd_pcm_close(capture->handle);
        free(capture);
        return -1;
    }

    /* Set sample rate */
    unsigned int rate = capture->sample_rate;
    err = snd_pcm_hw_params_set_rate_near(capture->handle, hw_params, &rate, 0);
    if (err < 0) {
        fprintf(stderr, "ERROR: Cannot set sample rate: %s\n",
                snd_strerror(err));
        snd_pcm_close(capture->handle);
        free(capture);
        return -1;
    }

    if (rate != (unsigned int)capture->sample_rate) {
        fprintf(stderr, "WARNING: Sample rate %u Hz (requested %d Hz)\n",
                rate, capture->sample_rate);
        capture->sample_rate = rate;
    }

    /* Set channels */
    err = snd_pcm_hw_params_set_channels(capture->handle, hw_params,
                                        capture->channels);
    if (err < 0) {
        fprintf(stderr, "ERROR: Cannot set channel count: %s\n",
                snd_strerror(err));
        snd_pcm_close(capture->handle);
        free(capture);
        return -1;
    }

    /* Set buffer size (3 frames = 15ms to prevent underruns) */
    snd_pcm_uframes_t buffer_size = capture->frame_size * 3;
    err = snd_pcm_hw_params_set_buffer_size_near(capture->handle, hw_params,
                                                 &buffer_size);
    if (err < 0) {
        fprintf(stderr, "WARNING: Cannot set buffer size: %s\n",
                snd_strerror(err));
    }

    /* Apply hardware parameters */
    err = snd_pcm_hw_params(capture->handle, hw_params);
    if (err < 0) {
        fprintf(stderr, "ERROR: Cannot apply hardware parameters: %s\n",
                snd_strerror(err));
        snd_pcm_close(capture->handle);
        free(capture);
        return -1;
    }

    /* Prepare device */
    err = snd_pcm_prepare(capture->handle);
    if (err < 0) {
        fprintf(stderr, "ERROR: Cannot prepare audio device: %s\n",
                snd_strerror(err));
        snd_pcm_close(capture->handle);
        free(capture);
        return -1;
    }

    capture->initialized = true;

    /* Store in context (reuse mouse fd field) */
    ctx->uinput_mouse_fd = (int)(intptr_t)capture;

    printf("✓ Audio capture ready: %d Hz, %d channels, %d samples/frame\n",
           capture->sample_rate, capture->channels, capture->frame_size);

    return 0;
}

/*
 * Capture one audio frame
 *
 * @param ctx         RootStream context
 * @param samples     Output PCM samples (interleaved stereo, 16-bit)
 * @param num_samples Output sample count
 * @return            0 on success, -1 on error
 */
int audio_capture_frame_alsa(rootstream_ctx_t *ctx, int16_t *samples,
                             size_t *num_samples) {
    if (!ctx || !samples || !num_samples) {
        return -1;
    }

    alsa_capture_ctx_t *capture = (alsa_capture_ctx_t*)(intptr_t)ctx->uinput_mouse_fd;
    if (!capture || !capture->initialized) {
        return -1;
    }

    /* Read PCM samples from device */
    snd_pcm_sframes_t frames = snd_pcm_readi(capture->handle, samples,
                                             capture->frame_size);

    if (frames < 0) {
        /* Handle ALSA errors */
        if (frames == -EPIPE) {
            /* Buffer overrun - recover */
            fprintf(stderr, "WARNING: Audio capture overrun, recovering\n");
            snd_pcm_prepare(capture->handle);
            return -1;
        } else if (frames == -ESTRPIPE) {
            /* Suspend - try to resume */
            fprintf(stderr, "WARNING: Audio capture suspended\n");
            while ((frames = snd_pcm_resume(capture->handle)) == -EAGAIN) {
                usleep(100);
            }
            if (frames < 0) {
                snd_pcm_prepare(capture->handle);
            }
            return -1;
        } else {
            fprintf(stderr, "ERROR: Audio capture failed: %s\n",
                    snd_strerror(frames));
            return -1;
        }
    }

    if (frames != capture->frame_size) {
        fprintf(stderr, "WARNING: Short read: %ld frames (expected %d)\n",
                frames, capture->frame_size);
    }

    *num_samples = frames;
    return 0;
}

/*
 * Cleanup audio capture
 */
void audio_capture_cleanup_alsa(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->uinput_mouse_fd) {
        return;
    }

    alsa_capture_ctx_t *capture = (alsa_capture_ctx_t*)(intptr_t)ctx->uinput_mouse_fd;

    if (capture->handle) {
        snd_pcm_drain(capture->handle);
        snd_pcm_close(capture->handle);
    }

    free(capture);
    ctx->uinput_mouse_fd = 0;

    printf("✓ Audio capture cleanup complete\n");
}

/* Backward compatibility wrappers */
int audio_capture_init(rootstream_ctx_t *ctx) {
    return audio_capture_init_alsa(ctx);
}

int audio_capture_frame(rootstream_ctx_t *ctx, int16_t *samples, size_t *num_samples) {
    return audio_capture_frame_alsa(ctx, samples, num_samples);
}

void audio_capture_cleanup(rootstream_ctx_t *ctx) {
    audio_capture_cleanup_alsa(ctx);
}
