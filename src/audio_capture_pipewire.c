/*
 * audio_capture_pipewire.c - PipeWire audio capture fallback
 * 
 * Works on modern Linux distributions where PipeWire is the default audio server.
 * Fedora 40+, Ubuntu 24.04+, Arch, etc.
 * 
 * Uses pw_stream for simple, non-blocking audio capture.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>

#ifdef HAVE_PIPEWIRE
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/props.h>
#include <spa/utils/result.h>

typedef struct {
    struct pw_loop *loop;
    struct pw_stream *stream;
    struct pw_core *core;
    struct pw_context *context;
    
    int16_t *buffer;
    size_t buffer_size;
    size_t read_pos;
    
    int sample_rate;
    int channels;
    int frame_size;
} pipewire_capture_ctx_t;

/* Stream events callback */
static void on_process(void *userdata) {
    pipewire_capture_ctx_t *pw = (pipewire_capture_ctx_t *)userdata;
    struct pw_buffer *b;
    struct spa_buffer *buf;

    if ((b = pw_stream_dequeue_buffer(pw->stream)) == NULL) {
        return;
    }

    buf = b->buffer;
    
    /* Get audio data from buffer */
    for (uint32_t i = 0; i < buf->n_datas; i++) {
        struct spa_data *d = &buf->datas[i];
        
        if (d->data == NULL) continue;
        
        uint32_t n_samples = d->chunk->size / sizeof(int16_t);
        int16_t *samples = (int16_t *)d->data;
        
        /* Copy to our buffer */
        if (pw->read_pos + n_samples <= pw->buffer_size) {
            memcpy(pw->buffer + pw->read_pos, samples, 
                   n_samples * sizeof(int16_t));
            pw->read_pos += n_samples;
        }
    }

    pw_stream_queue_buffer(pw->stream, b);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_process,
};

/*
 * Initialize PipeWire audio capture
 */
int audio_capture_init_pipewire(rootstream_ctx_t *ctx) {
    pipewire_capture_ctx_t *pw = calloc(1, sizeof(pipewire_capture_ctx_t));
    if (!pw) {
        fprintf(stderr, "ERROR: Memory allocation failed\n");
        return -1;
    }

    pw->sample_rate = 48000;
    pw->channels = 2;
    pw->frame_size = 240;  /* 5ms at 48kHz */
    pw->buffer_size = pw->frame_size * pw->channels * 4;  /* 4 frames buffer */

    pw->buffer = calloc(pw->buffer_size, sizeof(int16_t));
    if (!pw->buffer) {
        free(pw);
        return -1;
    }

    pw->read_pos = 0;

    /* Initialize PipeWire */
    pw_init(NULL, NULL);

    /* Create main loop */
    pw->loop = pw_loop_new(NULL);
    if (!pw->loop) {
        fprintf(stderr, "ERROR: Cannot create PipeWire main loop\n");
        free(pw->buffer);
        free(pw);
        return -1;
    }

    /* Create context */
    pw->context = pw_context_new(pw->loop, NULL, 0);
    if (!pw->context) {
        fprintf(stderr, "ERROR: Cannot create PipeWire context\n");
        pw_loop_destroy(pw->loop);
        free(pw->buffer);
        free(pw);
        return -1;
    }

    /* Create core and connect */
    pw->core = pw_context_connect(pw->context, NULL, 0);
    if (!pw->core) {
        fprintf(stderr, "ERROR: Cannot connect to PipeWire core\n");
        pw_context_destroy(pw->context);
        pw_loop_destroy(pw->loop);
        free(pw->buffer);
        free(pw);
        return -1;
    }

    /* Create stream */
    pw->stream = pw_stream_new_simple(
        pw->loop,
        "RootStream Capture",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Capture",
            PW_KEY_AUDIO_FORMAT, "S16LE",
            NULL
        ),
        &stream_events,
        pw
    );

    if (!pw->stream) {
        fprintf(stderr, "ERROR: Cannot create PipeWire stream\n");
        pw_core_disconnect(pw->core);
        pw_context_destroy(pw->context);
        pw_loop_destroy(pw->loop);
        free(pw->buffer);
        free(pw);
        return -1;
    }

    /* Build stream parameters */
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    
    const struct spa_pod *params[1];
    struct spa_audio_info_raw info = SPA_AUDIO_INFO_RAW_INIT(
        .format = SPA_AUDIO_FORMAT_S16,
        .channels = pw->channels,
        .rate = pw->sample_rate
    );
    
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);

    /* Connect stream for capture */
    if (pw_stream_connect(
            pw->stream,
            PW_DIRECTION_INPUT,
            PW_ID_ANY,
            PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS,
            params,
            1
        ) < 0) {
        fprintf(stderr, "ERROR: Cannot connect PipeWire stream\n");
        pw_stream_destroy(pw->stream);
        pw_core_disconnect(pw->core);
        pw_context_destroy(pw->context);
        pw_loop_destroy(pw->loop);
        free(pw->buffer);
        free(pw);
        return -1;
    }

    ctx->audio_capture_priv = pw;
    printf("✓ PipeWire audio capture initialized (48kHz, stereo)\n");
    return 0;
}

/*
 * Capture a frame via PipeWire
 */
int audio_capture_frame_pipewire(rootstream_ctx_t *ctx, int16_t *samples,
                                size_t *num_samples) {
    if (!ctx || !samples || !num_samples) return -1;
    
    pipewire_capture_ctx_t *pw = (pipewire_capture_ctx_t *)ctx->audio_capture_priv;
    if (!pw || !pw->stream) return -1;

    /* Run main loop to process events */
    pw_loop_iterate(pw->loop, 0);

    /* Check if we have enough samples */
    if (pw->read_pos < (size_t)(pw->frame_size * pw->channels)) {
        return -1;  /* Not enough data yet */
    }

    /* Copy samples */
    size_t bytes_to_copy = pw->frame_size * pw->channels * sizeof(int16_t);
    memcpy(samples, pw->buffer, bytes_to_copy);
    *num_samples = pw->frame_size * pw->channels;

    /* Shift remaining data */
    if (pw->read_pos > (size_t)(pw->frame_size * pw->channels)) {
        memmove(pw->buffer, 
               pw->buffer + pw->frame_size * pw->channels,
               (pw->read_pos - pw->frame_size * pw->channels) * sizeof(int16_t));
        pw->read_pos -= pw->frame_size * pw->channels;
    } else {
        pw->read_pos = 0;
    }

    return 0;
}

/*
 * Cleanup PipeWire capture
 */
void audio_capture_cleanup_pipewire(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->audio_capture_priv) return;
    
    pipewire_capture_ctx_t *pw = (pipewire_capture_ctx_t *)ctx->audio_capture_priv;
    
    if (pw->stream) pw_stream_destroy(pw->stream);
    if (pw->core) pw_core_disconnect(pw->core);
    if (pw->context) pw_context_destroy(pw->context);
    if (pw->loop) pw_loop_destroy(pw->loop);
    if (pw->buffer) free(pw->buffer);
    
    free(pw);
    ctx->audio_capture_priv = NULL;
}

/*
 * Check if PipeWire is available
 */
bool audio_capture_pipewire_available(void) {
    /* Try to connect to PipeWire daemon */
    pw_init(NULL, NULL);
    
    struct pw_loop *loop = pw_loop_new(NULL);
    if (!loop) return false;
    
    struct pw_context *context = pw_context_new(loop, NULL, 0);
    if (!context) {
        pw_loop_destroy(loop);
        return false;
    }
    
    struct pw_core *core = pw_context_connect(context, NULL, 0);
    bool available = (core != NULL);
    
    if (core) pw_core_disconnect(core);
    pw_context_destroy(context);
    pw_loop_destroy(loop);
    
    return available;
}

#else

/* Stub for NO_PIPEWIRE builds */
int audio_capture_init_pipewire(rootstream_ctx_t *ctx) {
    (void)ctx;
    return -1;
}

int audio_capture_frame_pipewire(rootstream_ctx_t *ctx, int16_t *samples,
                                size_t *num_samples) {
    (void)ctx;
    (void)samples;
    (void)num_samples;
    return -1;
}

void audio_capture_cleanup_pipewire(rootstream_ctx_t *ctx) {
    (void)ctx;
}

bool audio_capture_pipewire_available(void) {
    return false;
}

#endif
