/*
 * opus_codec.c - Opus audio codec for low-latency streaming
 *
 * Opus is designed specifically for low-latency audio streaming.
 * - 5-20ms algorithmic delay
 * - Excellent quality at 64kbps
 * - Used by Discord, WebRTC, Mumble
 * - MIT licensed, no patent issues
 *
 * Architecture:
 * - 48000 Hz sample rate (Opus native)
 * - 2 channels (stereo)
 * - 240 samples per frame (5ms at 48kHz)
 * - 64kbps bitrate (good quality, low bandwidth)
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <opus/opus.h>

/* Opus parameters */
#define OPUS_SAMPLE_RATE    48000
#define OPUS_CHANNELS       2
#define OPUS_FRAME_SIZE     240      /* 5ms at 48kHz */
#define OPUS_BITRATE        64000    /* 64 kbps */
#define OPUS_APPLICATION    OPUS_APPLICATION_RESTRICTED_LOWDELAY

typedef struct {
    OpusEncoder *encoder;
    OpusDecoder *decoder;
    int sample_rate;
    int channels;
    int frame_size;
    int bitrate;
} opus_ctx_t;

/*
 * Initialize Opus encoder
 *
 * @param ctx RootStream context
 * @return    0 on success, -1 on error
 */
int rootstream_opus_encoder_init(rootstream_ctx_t *ctx) {
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context for Opus encoder\n");
        return -1;
    }

    /* Allocate Opus context */
    opus_ctx_t *opus = calloc(1, sizeof(opus_ctx_t));
    if (!opus) {
        fprintf(stderr, "ERROR: Cannot allocate Opus context\n");
        return -1;
    }

    opus->sample_rate = OPUS_SAMPLE_RATE;
    opus->channels = OPUS_CHANNELS;
    opus->frame_size = OPUS_FRAME_SIZE;
    opus->bitrate = ctx->settings.audio_bitrate;
    if (opus->bitrate == 0) {
        opus->bitrate = OPUS_BITRATE;
    }

    /* Create Opus encoder */
    int error;
    opus->encoder = opus_encoder_create(
        opus->sample_rate,
        opus->channels,
        OPUS_APPLICATION,
        &error
    );

    if (error != OPUS_OK || !opus->encoder) {
        fprintf(stderr, "ERROR: Opus encoder creation failed: %s\n",
                opus_strerror(error));
        free(opus);
        return -1;
    }

    /* Configure encoder for low latency */
    opus_encoder_ctl(opus->encoder, OPUS_SET_BITRATE(opus->bitrate));
    opus_encoder_ctl(opus->encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
    opus_encoder_ctl(opus->encoder, OPUS_SET_VBR(0));  /* CBR for consistent latency */
    opus_encoder_ctl(opus->encoder, OPUS_SET_COMPLEXITY(5));  /* Balance quality/speed */

    /* Store in context (reuse input.c fd field) */
    ctx->uinput_kbd_fd = (int)(intptr_t)opus;

    printf("✓ Opus encoder ready: %d Hz, %d channels, %d kbps\n",
           opus->sample_rate, opus->channels, opus->bitrate / 1000);

    return 0;
}

/*
 * Initialize Opus decoder
 *
 * @param ctx RootStream context
 * @return    0 on success, -1 on error
 */
int rootstream_opus_decoder_init(rootstream_ctx_t *ctx) {
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context for Opus decoder\n");
        return -1;
    }

    /* Get Opus context (or create if encoder not initialized) */
    opus_ctx_t *opus = (opus_ctx_t*)(intptr_t)ctx->uinput_kbd_fd;
    if (!opus) {
        opus = calloc(1, sizeof(opus_ctx_t));
        if (!opus) {
            fprintf(stderr, "ERROR: Cannot allocate Opus context\n");
            return -1;
        }
        opus->sample_rate = OPUS_SAMPLE_RATE;
        opus->channels = OPUS_CHANNELS;
        opus->frame_size = OPUS_FRAME_SIZE;
    opus->bitrate = ctx->settings.audio_bitrate;
    if (opus->bitrate == 0) {
        opus->bitrate = OPUS_BITRATE;
    }
        ctx->uinput_kbd_fd = (int)(intptr_t)opus;
    }

    /* Create Opus decoder */
    int error;
    opus->decoder = opus_decoder_create(
        opus->sample_rate,
        opus->channels,
        &error
    );

    if (error != OPUS_OK || !opus->decoder) {
        fprintf(stderr, "ERROR: Opus decoder creation failed: %s\n",
                opus_strerror(error));
        if (!opus->encoder) {
            free(opus);
            ctx->uinput_kbd_fd = 0;
        }
        return -1;
    }

    printf("✓ Opus decoder ready: %d Hz, %d channels\n",
           opus->sample_rate, opus->channels);

    return 0;
}

/*
 * Encode PCM audio to Opus
 *
 * @param ctx     RootStream context
 * @param pcm     Input PCM samples (interleaved stereo, 16-bit)
 * @param out     Output Opus packet
 * @param out_len Output packet length
 * @return        0 on success, -1 on error
 */
int rootstream_opus_encode(rootstream_ctx_t *ctx, const int16_t *pcm,
               uint8_t *out, size_t *out_len) {
    if (!ctx || !pcm || !out || !out_len) {
        return -1;
    }

    opus_ctx_t *opus = (opus_ctx_t*)(intptr_t)ctx->uinput_kbd_fd;
    if (!opus || !opus->encoder) {
        fprintf(stderr, "ERROR: Opus encoder not initialized\n");
        return -1;
    }

    /* Encode frame */
    int encoded_bytes = opus_encode(
        opus->encoder,
        pcm,
        opus->frame_size,
        out,
        4000  /* Max packet size */
    );

    if (encoded_bytes < 0) {
        fprintf(stderr, "ERROR: Opus encode failed: %s\n",
                opus_strerror(encoded_bytes));
        return -1;
    }

    *out_len = encoded_bytes;
    return 0;
}

/*
 * Decode Opus to PCM audio
 *
 * @param ctx     RootStream context
 * @param in      Input Opus packet
 * @param in_len  Input packet length
 * @param pcm     Output PCM samples (interleaved stereo, 16-bit)
 * @param pcm_len Output sample count
 * @return        0 on success, -1 on error
 */
int rootstream_opus_decode(rootstream_ctx_t *ctx, const uint8_t *in, size_t in_len,
               int16_t *pcm, size_t *pcm_len) {
    if (!ctx || !in || !pcm || !pcm_len) {
        return -1;
    }

    opus_ctx_t *opus = (opus_ctx_t*)(intptr_t)ctx->uinput_kbd_fd;
    if (!opus || !opus->decoder) {
        fprintf(stderr, "ERROR: Opus decoder not initialized\n");
        return -1;
    }

    /* Decode frame */
    int decoded_samples = opus_decode(
        opus->decoder,
        in,
        in_len,
        pcm,
        5760,  /* Max frame size (120ms at 48kHz) */
        0      /* No FEC */
    );

    if (decoded_samples < 0) {
        fprintf(stderr, "ERROR: Opus decode failed: %s\n",
                opus_strerror(decoded_samples));
        return -1;
    }

    *pcm_len = decoded_samples;
    return 0;
}

/*
 * Cleanup Opus encoder/decoder
 */
void rootstream_opus_cleanup(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->uinput_kbd_fd) {
        return;
    }

    opus_ctx_t *opus = (opus_ctx_t*)(intptr_t)ctx->uinput_kbd_fd;

    if (opus->encoder) {
        opus_encoder_destroy(opus->encoder);
    }

    if (opus->decoder) {
        opus_decoder_destroy(opus->decoder);
    }

    free(opus);
    ctx->uinput_kbd_fd = 0;

    printf("✓ Opus codec cleanup complete\n");
}

/*
 * Get Opus frame size (samples per channel)
 */
int rootstream_opus_get_frame_size(void) {
    return OPUS_FRAME_SIZE;
}

/*
 * Get Opus sample rate
 */
int rootstream_opus_get_sample_rate(void) {
    return OPUS_SAMPLE_RATE;
}

/*
 * Get Opus channels
 */
int rootstream_opus_get_channels(void) {
    return OPUS_CHANNELS;
}
