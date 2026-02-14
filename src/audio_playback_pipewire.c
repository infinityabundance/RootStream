/*
 * audio_playback_pipewire.c - PipeWire audio playback fallback
 * 
 * Works on modern Linux distributions where PipeWire is the default.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#ifdef HAVE_PIPEWIRE
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/utils/result.h>

typedef struct {
    struct pw_loop *loop;
    struct pw_stream *stream;
    struct pw_core *core;
    struct pw_context *context;
    
    int sample_rate;
    int channels;
} pipewire_playback_ctx_t;

/* Stream events */
static void on_playback_process(void *userdata) {
    pipewire_playback_ctx_t *pw = (pipewire_playback_ctx_t *)userdata;
    struct pw_buffer *b;
    
    if ((b = pw_stream_dequeue_buffer(pw->stream)) == NULL)
        return;

    pw_stream_queue_buffer(pw->stream, b);
}

static const struct pw_stream_events playback_stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_playback_process,
};

/*
 * Initialize PipeWire audio playback
 */
int audio_playback_init_pipewire(rootstream_ctx_t *ctx) {
    pipewire_playback_ctx_t *pw = calloc(1, sizeof(pipewire_playback_ctx_t));
    if (!pw) return -1;

    pw->sample_rate = 48000;
    pw->channels = 2;

    /* Initialize PipeWire (will be cleaned up in cleanup function) */
    pw_init(NULL, NULL);

    /* Create main loop */
    pw->loop = pw_loop_new(NULL);
    if (!pw->loop) {
        pw_deinit();
        free(pw);
        return -1;
    }

    /* Create context */
    pw->context = pw_context_new(pw->loop, NULL, 0);
    if (!pw->context) {
        pw_loop_destroy(pw->loop);
        pw_deinit();
        free(pw);
        return -1;
    }

    /* Connect to core */
    pw->core = pw_context_connect(pw->context, NULL, 0);
    if (!pw->core) {
        pw_context_destroy(pw->context);
        pw_loop_destroy(pw->loop);
        pw_deinit();
        free(pw);
        return -1;
    }

    /* Create playback stream */
    pw->stream = pw_stream_new_simple(
        pw->loop,
        "RootStream Playback",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Playback",
            PW_KEY_AUDIO_FORMAT, "S16LE",
            NULL
        ),
        &playback_stream_events,
        pw
    );

    if (!pw->stream) {
        pw_core_disconnect(pw->core);
        pw_context_destroy(pw->context);
        pw_loop_destroy(pw->loop);
        pw_deinit();
        free(pw);
        return -1;
    }

    /* Build stream parameters */
    uint8_t params_buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(params_buffer, sizeof(params_buffer));
    
    const struct spa_pod *params[1];
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, 
        &SPA_AUDIO_INFO_RAW_INIT(
            .format = SPA_AUDIO_FORMAT_S16,
            .channels = pw->channels,
            .rate = pw->sample_rate
        ));

    /* Connect stream for playback */
    if (pw_stream_connect(
            pw->stream,
            PW_DIRECTION_OUTPUT,
            PW_ID_ANY,
            PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS,
            params,
            1
        ) < 0) {
        pw_stream_destroy(pw->stream);
        pw_core_disconnect(pw->core);
        pw_context_destroy(pw->context);
        pw_loop_destroy(pw->loop);
        pw_deinit();
        free(pw);
        return -1;
    }

    ctx->audio_playback_priv = pw;
    printf("✓ PipeWire audio playback initialized\n");
    return 0;
}

/*
 * Write audio samples via PipeWire
 */
int audio_playback_write_pipewire(rootstream_ctx_t *ctx, const int16_t *samples,
                                 size_t num_samples) {
    if (!ctx || !samples || num_samples == 0) return 0;
    
    pipewire_playback_ctx_t *pw = (pipewire_playback_ctx_t *)ctx->audio_playback_priv;
    if (!pw || !pw->stream) return -1;

    struct pw_buffer *b = pw_stream_dequeue_buffer(pw->stream);
    if (!b) return -1;

    struct spa_buffer *buf = b->buffer;
    
    /* Copy samples to buffer */
    for (uint32_t i = 0; i < buf->n_datas; i++) {
        struct spa_data *d = &buf->datas[i];
        size_t size = num_samples * sizeof(int16_t);
        
        if (d->maxsize >= size) {
            memcpy(d->data, samples, size);
            d->chunk->size = size;
            d->chunk->offset = 0;
            d->chunk->stride = sizeof(int16_t);
        }
    }

    pw_stream_queue_buffer(pw->stream, b);
    return 0;
}

/*
 * Cleanup PipeWire playback
 */
void audio_playback_cleanup_pipewire(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->audio_playback_priv) return;
    
    pipewire_playback_ctx_t *pw = (pipewire_playback_ctx_t *)ctx->audio_playback_priv;
    
    if (pw->stream) pw_stream_destroy(pw->stream);
    if (pw->core) pw_core_disconnect(pw->core);
    if (pw->context) pw_context_destroy(pw->context);
    if (pw->loop) pw_loop_destroy(pw->loop);
    
    pw_deinit();
    
    free(pw);
    ctx->audio_playback_priv = NULL;
}

/*
 * Check if PipeWire is available
 */
bool audio_playback_pipewire_available(void) {
    pw_init(NULL, NULL);
    
    struct pw_loop *loop = pw_loop_new(NULL);
    if (!loop) {
        pw_deinit();
        return false;
    }
    
    struct pw_context *context = pw_context_new(loop, NULL, 0);
    if (!context) {
        pw_loop_destroy(loop);
        pw_deinit();
        return false;
    }
    
    struct pw_core *core = pw_context_connect(context, NULL, 0);
    bool available = (core != NULL);
    
    if (core) pw_core_disconnect(core);
    pw_context_destroy(context);
    pw_loop_destroy(loop);
    pw_deinit();
    
    return available;
}

#else

int audio_playback_init_pipewire(rootstream_ctx_t *ctx) {
    (void)ctx;
    return -1;
}

int audio_playback_write_pipewire(rootstream_ctx_t *ctx, const int16_t *samples,
                                 size_t num_samples) {
    (void)ctx;
    (void)samples;
    (void)num_samples;
    return -1;
}

void audio_playback_cleanup_pipewire(rootstream_ctx_t *ctx) {
    (void)ctx;
}

bool audio_playback_pipewire_available(void) {
    return false;
}

#endif
