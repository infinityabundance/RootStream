/* PipeWire Playback Implementation */
#include "playback_pipewire.h"

#ifdef HAVE_PIPEWIRE

#include <cstdio>
#include <cstring>
#include <pipewire/pipewire.h>

PipeWirePlayback::PipeWirePlayback()
    : loop(nullptr), stream(nullptr), sample_rate(0), channels(0), playing(false) {
}

PipeWirePlayback::~PipeWirePlayback() {
    cleanup();
}

// Stream events callback
static void on_process(void *userdata) {
    // This is called when PipeWire needs more audio data
    // For now, we'll handle this in write_samples
    (void)userdata;
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_process,
};

int PipeWirePlayback::init(int sample_rate, int channels, const char *device) {
    this->sample_rate = sample_rate;
    this->channels = channels;
    
    // Initialize PipeWire
    pw_init(nullptr, nullptr);
    
    // Create thread loop
    loop = pw_thread_loop_new("rootstream-audio", nullptr);
    if (!loop) {
        fprintf(stderr, "Failed to create PipeWire thread loop\n");
        return -1;
    }
    
    // Get loop context
    struct pw_loop *pw_loop = pw_thread_loop_get_loop(loop);
    struct pw_context *context = pw_context_new(pw_loop, nullptr, 0);
    if (!context) {
        fprintf(stderr, "Failed to create PipeWire context\n");
        cleanup();
        return -1;
    }
    
    // Start the loop
    if (pw_thread_loop_start(loop) < 0) {
        fprintf(stderr, "Failed to start PipeWire loop\n");
        cleanup();
        return -1;
    }
    
    // Lock for stream creation
    pw_thread_loop_lock(loop);
    
    // Create stream properties
    struct pw_properties *props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Playback",
        PW_KEY_MEDIA_ROLE, "Game",
        PW_KEY_APP_NAME, "RootStream",
        nullptr);
    
    if (device) {
        pw_properties_set(props, PW_KEY_NODE_TARGET, device);
    }
    
    // Create stream
    stream = pw_stream_new_simple(
        pw_loop,
        "rootstream-playback",
        props,
        &stream_events,
        this);
    
    if (!stream) {
        fprintf(stderr, "Failed to create PipeWire stream\n");
        pw_thread_loop_unlock(loop);
        cleanup();
        return -1;
    }
    
    // Setup audio format
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    
    const struct spa_pod *params[1];
    params[0] = (struct spa_pod*)spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
        &SPA_AUDIO_INFO_RAW_INIT(
            .format = SPA_AUDIO_FORMAT_F32,
            .rate = (uint32_t)sample_rate,
            .channels = (uint32_t)channels));
    
    // Connect stream
    if (pw_stream_connect(stream,
                         PW_DIRECTION_OUTPUT,
                         PW_ID_ANY,
                         (enum pw_stream_flags)(
                             PW_STREAM_FLAG_AUTOCONNECT |
                             PW_STREAM_FLAG_MAP_BUFFERS |
                             PW_STREAM_FLAG_RT_PROCESS),
                         params, 1) < 0) {
        fprintf(stderr, "Failed to connect PipeWire stream\n");
        pw_thread_loop_unlock(loop);
        cleanup();
        return -1;
    }
    
    pw_thread_loop_unlock(loop);
    
    fprintf(stderr, "PipeWire audio initialized: %d Hz, %d channels\n", 
            sample_rate, channels);
    return 0;
}

int PipeWirePlayback::start_playback() {
    if (!stream) {
        return -1;
    }
    
    pw_thread_loop_lock(loop);
    pw_stream_set_active(stream, true);
    pw_thread_loop_unlock(loop);
    
    playing = true;
    return 0;
}

int PipeWirePlayback::stop_playback() {
    if (!stream) {
        return -1;
    }
    
    pw_thread_loop_lock(loop);
    pw_stream_set_active(stream, false);
    pw_thread_loop_unlock(loop);
    
    playing = false;
    return 0;
}

int PipeWirePlayback::pause_playback() {
    return stop_playback();
}

int PipeWirePlayback::resume_playback() {
    return start_playback();
}

int PipeWirePlayback::write_samples(const float *samples, int sample_count) {
    if (!stream || !playing) {
        return -1;
    }
    
    pw_thread_loop_lock(loop);
    
    struct pw_buffer *b = pw_stream_dequeue_buffer(stream);
    if (!b) {
        pw_thread_loop_unlock(loop);
        return 0;  // No buffer available
    }
    
    struct spa_buffer *buf = b->buffer;
    struct spa_data *d = &buf->datas[0];
    
    if (!d->data) {
        pw_stream_queue_buffer(stream, b);
        pw_thread_loop_unlock(loop);
        return 0;
    }
    
    // Copy audio data
    size_t bytes_to_copy = sample_count * sizeof(float);
    size_t max_size = d->maxsize;
    
    if (bytes_to_copy > max_size) {
        bytes_to_copy = max_size;
    }
    
    memcpy(d->data, samples, bytes_to_copy);
    d->chunk->offset = 0;
    d->chunk->stride = sizeof(float) * channels;
    d->chunk->size = bytes_to_copy;
    
    pw_stream_queue_buffer(stream, b);
    pw_thread_loop_unlock(loop);
    
    return bytes_to_copy / sizeof(float);
}

int PipeWirePlayback::get_buffer_latency_ms() {
    // TODO: Query actual latency from PipeWire
    return 50;  // Estimate
}

int PipeWirePlayback::set_volume(float percent) {
    (void)percent;
    // TODO: Implement volume control via PipeWire
    return 0;
}

float PipeWirePlayback::get_volume() {
    return 1.0f;
}

void PipeWirePlayback::cleanup() {
    if (stream) {
        pw_thread_loop_lock(loop);
        pw_stream_destroy(stream);
        stream = nullptr;
        pw_thread_loop_unlock(loop);
    }
    
    if (loop) {
        pw_thread_loop_stop(loop);
        pw_thread_loop_destroy(loop);
        loop = nullptr;
    }
    
    playing = false;
    pw_deinit();
}

#endif // HAVE_PIPEWIRE
