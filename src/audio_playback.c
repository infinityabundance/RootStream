/*
 * audio_playback.c - ALSA audio playback for client
 *
 * Plays decoded audio using ALSA (Advanced Linux Sound Architecture).
 * Configured for low-latency playback to minimize audio lag.
 *
 * Parameters:
 * - 48000 Hz sample rate
 * - 2 channels (stereo)
 * - 16-bit signed PCM
 * - Small buffer for low latency
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <alsa/asoundlib.h>

#define SAMPLE_RATE 48000
#define CHANNELS 2

typedef struct {
    snd_pcm_t *handle;
    int sample_rate;
    int channels;
    bool initialized;
} audio_playback_ctx_t;

/*
 * Check if ALSA is available
 */
bool audio_playback_alsa_available(void) {
    /* Try to open and immediately close a test handle */
    snd_pcm_t *handle = NULL;
    int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err >= 0 && handle) {
        snd_pcm_close(handle);
        return true;
    }
    return false;
}

/*
 * Initialize ALSA audio playback
 *
 * @param ctx RootStream context
 * @return    0 on success, -1 on error
 */
int audio_playback_init_alsa(rootstream_ctx_t *ctx) {
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context for audio playback\n");
        return -1;
    }

    /* Allocate playback context */
    audio_playback_ctx_t *playback = calloc(1, sizeof(audio_playback_ctx_t));
    if (!playback) {
        fprintf(stderr, "ERROR: Cannot allocate audio playback context\n");
        return -1;
    }

    playback->sample_rate = SAMPLE_RATE;
    playback->channels = CHANNELS;

    /* Open ALSA device for playback */
    int err = snd_pcm_open(&playback->handle, "default",
                          SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        fprintf(stderr, "ERROR: Cannot open audio playback device: %s\n",
                snd_strerror(err));
        free(playback);
        return -1;
    }

    /* Configure hardware parameters */
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(playback->handle, hw_params);

    /* Set access type (interleaved) */
    err = snd_pcm_hw_params_set_access(playback->handle, hw_params,
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        fprintf(stderr, "ERROR: Cannot set audio access type: %s\n",
                snd_strerror(err));
        snd_pcm_close(playback->handle);
        free(playback);
        return -1;
    }

    /* Set sample format (16-bit signed) */
    err = snd_pcm_hw_params_set_format(playback->handle, hw_params,
                                       SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
        fprintf(stderr, "ERROR: Cannot set audio format: %s\n",
                snd_strerror(err));
        snd_pcm_close(playback->handle);
        free(playback);
        return -1;
    }

    /* Set sample rate */
    unsigned int rate = playback->sample_rate;
    err = snd_pcm_hw_params_set_rate_near(playback->handle, hw_params, &rate, 0);
    if (err < 0) {
        fprintf(stderr, "ERROR: Cannot set sample rate: %s\n",
                snd_strerror(err));
        snd_pcm_close(playback->handle);
        free(playback);
        return -1;
    }

    if (rate != (unsigned int)playback->sample_rate) {
        fprintf(stderr, "WARNING: Playback rate %u Hz (requested %d Hz)\n",
                rate, playback->sample_rate);
        playback->sample_rate = rate;
    }

    /* Set channels */
    err = snd_pcm_hw_params_set_channels(playback->handle, hw_params,
                                        playback->channels);
    if (err < 0) {
        fprintf(stderr, "ERROR: Cannot set channel count: %s\n",
                snd_strerror(err));
        snd_pcm_close(playback->handle);
        free(playback);
        return -1;
    }

    /* Set period size (240 samples = 5ms at 48kHz) */
    snd_pcm_uframes_t period_size = 240;
    err = snd_pcm_hw_params_set_period_size_near(playback->handle, hw_params,
                                                 &period_size, NULL);
    if (err < 0) {
        fprintf(stderr, "WARNING: Cannot set period size: %s\n",
                snd_strerror(err));
    }

    /* Set buffer size (4 periods = 20ms) */
    snd_pcm_uframes_t buffer_size = period_size * 4;
    err = snd_pcm_hw_params_set_buffer_size_near(playback->handle, hw_params,
                                                 &buffer_size);
    if (err < 0) {
        fprintf(stderr, "WARNING: Cannot set buffer size: %s\n",
                snd_strerror(err));
    }

    /* Apply hardware parameters */
    err = snd_pcm_hw_params(playback->handle, hw_params);
    if (err < 0) {
        fprintf(stderr, "ERROR: Cannot apply hardware parameters: %s\n",
                snd_strerror(err));
        snd_pcm_close(playback->handle);
        free(playback);
        return -1;
    }

    /* Prepare device */
    err = snd_pcm_prepare(playback->handle);
    if (err < 0) {
        fprintf(stderr, "ERROR: Cannot prepare audio device: %s\n",
                snd_strerror(err));
        snd_pcm_close(playback->handle);
        free(playback);
        return -1;
    }

    playback->initialized = true;

    /* Store in context (will need dedicated field in Phase 2) */
    /* For now, reuse a pointer - proper integration in rootstream.h update */
    ctx->tray.menu = playback;

    printf("✓ Audio playback ready: %d Hz, %d channels\n",
           playback->sample_rate, playback->channels);

    return 0;
}

/*
 * Play audio samples
 *
 * @param ctx         RootStream context
 * @param samples     PCM samples (interleaved stereo, 16-bit)
 * @param num_samples Sample count per channel
 * @return            0 on success, -1 on error
 */
int audio_playback_write_alsa(rootstream_ctx_t *ctx, int16_t *samples,
                              size_t num_samples) {
    if (!ctx || !samples || num_samples == 0) {
        return -1;
    }

    audio_playback_ctx_t *playback = (audio_playback_ctx_t*)ctx->tray.menu;
    if (!playback || !playback->initialized) {
        return -1;
    }

    /* Write PCM samples to device */
    snd_pcm_sframes_t frames = snd_pcm_writei(playback->handle, samples,
                                              num_samples);

    if (frames < 0) {
        /* Handle ALSA errors */
        if (frames == -EPIPE) {
            /* Buffer underrun - recover */
            fprintf(stderr, "WARNING: Audio playback underrun, recovering\n");
            snd_pcm_prepare(playback->handle);
            return -1;
        } else if (frames == -ESTRPIPE) {
            /* Suspend - try to resume */
            fprintf(stderr, "WARNING: Audio playback suspended\n");
            while ((frames = snd_pcm_resume(playback->handle)) == -EAGAIN) {
                usleep(100);
            }
            if (frames < 0) {
                snd_pcm_prepare(playback->handle);
            }
            return -1;
        } else {
            fprintf(stderr, "ERROR: Audio playback failed: %s\n",
                    snd_strerror(frames));
            return -1;
        }
    }

    if (frames != (snd_pcm_sframes_t)num_samples) {
        fprintf(stderr, "WARNING: Short write: %ld frames (expected %zu)\n",
                frames, num_samples);
    }

    return 0;
}

/*
 * Cleanup audio playback
 */
void audio_playback_cleanup_alsa(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->tray.menu) {
        return;
    }

    audio_playback_ctx_t *playback = (audio_playback_ctx_t*)ctx->tray.menu;

    if (playback->handle) {
        snd_pcm_drain(playback->handle);
        snd_pcm_close(playback->handle);
    }

    free(playback);
    ctx->tray.menu = NULL;

    printf("✓ Audio playback cleanup complete\n");
}

/* Backward compatibility wrappers */
int audio_playback_init(rootstream_ctx_t *ctx) {
    return audio_playback_init_alsa(ctx);
}

int audio_playback_write(rootstream_ctx_t *ctx, int16_t *samples, size_t num_samples) {
    return audio_playback_write_alsa(ctx, samples, num_samples);
}

void audio_playback_cleanup(rootstream_ctx_t *ctx) {
    audio_playback_cleanup_alsa(ctx);
}
