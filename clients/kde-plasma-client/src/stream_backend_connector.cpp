/*
 * stream_backend_connector.cpp — Implementation of StreamBackendConnector
 *
 * PHASE-95.3 REWRITE
 * ------------------
 * The previous implementation used network_client_t (the duplicate local
 * protocol implementation that was never wired to the real backend) and
 * drove the Vulkan pipeline directly from the callback.
 *
 * This implementation:
 *   1. Uses rs_client_session_t — the real rootstream_core backend.
 *   2. Runs the session on a dedicated QThread (same thread model as before,
 *      but now it's a QThread rather than a raw pthread).
 *   3. Emits Qt signals (QueuedConnection) so VideoRenderer and AudioPlayer
 *      receive frames on the correct thread.
 *   4. Removes the namespace RootStream:: wrapper (StreamBackendConnector is
 *      a QObject subclass and QObjects cannot live in namespaces that have
 *      their own QMetaObject conflicts in MOC output).
 *
 * WHAT HAPPENED TO THE VULKAN DIRECT PATH?
 * -----------------------------------------
 * The old direct vulkan_upload_frame / vulkan_render / vulkan_present calls
 * have been removed.  The renderer is now driven by VideoRenderer (a
 * QQuickFramebufferObject) which runs on the Qt render thread.  Bypassing
 * Qt's render thread would cause GL/Vulkan command buffer ordering issues.
 *
 * The new flow is:
 *   C callback → copy frame → emit videoFrameReady signal
 *   → VideoRenderer::submitFrame (on GUI thread, via QueuedConnection)
 *   → VideoRenderer::Renderer::render() (on Qt render thread)
 *   → GL texture upload + shader draw
 *
 * This is correct because the Qt Quick scene graph owns the render thread.
 */

#include "stream_backend_connector.h"

#include <QDebug>
#include <cstring>

/* ── Constructor / Destructor ─────────────────────────────────────── */

StreamBackendConnector::StreamBackendConnector(QObject *parent)
    : QObject(parent)
{}

StreamBackendConnector::~StreamBackendConnector() {
    /* Ensure clean shutdown even if the caller forgot to call disconnect() */
    disconnect();
}

/* ── Public API ───────────────────────────────────────────────────── */

void StreamBackendConnector::connectToHost(const QString &host,
                                            int            port,
                                            const QString &code)
{
    /* If a session is already running, stop it first (implicit reconnect) */
    if (session_) disconnect();

    host_ = host;
    port_ = port;
    code_ = code;

    /* Build the C session configuration */
    rs_client_config_t cfg = {};
    /* Store host as a QByteArray so the C string pointer remains valid
     * for the lifetime of the session.  The config struct is copied by
     * rs_client_session_create(), but the peer_host pointer must remain
     * valid until rs_client_session_destroy(). */
    QByteArray host_bytes = host.toUtf8();
    cfg.peer_host     = host_bytes.constData();
    cfg.peer_port     = port;
    cfg.audio_enabled = true;
    cfg.low_latency   = true;

    session_ = rs_client_session_create(&cfg);
    if (!session_) {
        qWarning() << "StreamBackendConnector: failed to create session";
        emit connectionStateChanged(QStringLiteral("error: session create failed"));
        return;
    }

    /* Register the C static callback trampolines */
    rs_client_session_set_video_callback(session_, cVideoCallback, this);
    rs_client_session_set_audio_callback(session_, cAudioCallback, this);
    rs_client_session_set_state_callback(session_, cStateCallback, this);

    /* Create a QThread and run the session loop on it.
     * We use a lambda rather than subclassing QThread — the lambda captures
     * the session pointer and calls run() which blocks until the session ends.
     *
     * Why not QThread::create()?  QThread::create() was introduced in Qt 5.10
     * and requires a function object.  Using a lambda here is identical but
     * more explicit about what is happening. */
    thread_ = new QThread(this);

    /* Move the session run call into the thread via a QObject::connect to
     * the thread's started() signal.  We use a direct connection because
     * the connector itself lives on the GUI thread and the lambda runs on
     * the worker thread. */
    connect(thread_, &QThread::started, this, [this, host_bytes]() mutable {
        /* host_bytes is kept alive by capture so cfg.peer_host remains valid */
        int rc = rs_client_session_run(session_);
        if (rc != 0) {
            qWarning() << "StreamBackendConnector: session run returned" << rc;
        }
        /* Signal the GUI thread that the session has ended */
        emit sessionStopped();
        thread_->quit();
    }, Qt::DirectConnection);

    connect(thread_, &QThread::finished, thread_, &QThread::deleteLater);
    connect(thread_, &QThread::finished, this, [this]() {
        thread_ = nullptr;
    });

    thread_->start();
}

void StreamBackendConnector::disconnect() {
    if (!session_) return;

    /* Request the session loop to stop.  The loop checks stop_requested at
     * the top of every iteration (max latency: one recv poll = ~16ms). */
    rs_client_session_request_stop(session_);

    /* Wait for the thread to exit.  If it has already been deleted by
     * QThread::deleteLater, thread_ will be nullptr. */
    if (thread_) {
        thread_->wait(2000 /* ms timeout */);
        /* If the thread is still running after 2s, terminate it.
         * This should not happen in normal operation but is a safety net
         * against a stuck network recv() call. */
        if (thread_->isRunning()) {
            qWarning() << "StreamBackendConnector: thread did not exit cleanly, terminating";
            thread_->terminate();
            thread_->wait();
        }
    }

    rs_client_session_destroy(session_);
    session_ = nullptr;
}

bool StreamBackendConnector::isConnected() const {
    return session_ && rs_client_session_is_running(session_);
}

QString StreamBackendConnector::decoderName() const {
    if (!session_) return QStringLiteral("unknown");
    return QString::fromUtf8(rs_client_session_decoder_name(session_));
}

/* ── C callback trampolines ───────────────────────────────────────── */

/*
 * cVideoCallback — called from the session worker thread each decoded frame.
 *
 * FRAME COPY STRATEGY (MVP)
 * -------------------------
 * We copy the NV12 data into a QByteArray immediately before returning.
 * This is a CPU memcpy of width*height*1.5 bytes (e.g., 1920×1080 = ~3MB).
 * At 60fps that is ~180MB/s — fast enough for current hardware.
 *
 * ZERO-COPY UPGRADE PATH
 * ----------------------
 * Replace this memcpy with a DMABUF handle export from the VA-API decoder.
 * The VideoRenderer would then import the DMABUF as an EGL image via
 * EGL_EXT_image_dma_buf_import, skipping the CPU copy entirely.
 * See docs/architecture/client_session_api.md for the upgrade plan.
 */
void StreamBackendConnector::cVideoCallback(void                  *user,
                                             const rs_video_frame_t *frame)
{
    auto *self = static_cast<StreamBackendConnector *>(user);
    if (!self || !frame || !frame->plane0) return;

    int w = frame->width;
    int h = frame->height;

    /* NV12 size: Y plane (w*h bytes) + UV plane (w*h/2 bytes) = w*h*3/2 */
    int nv12_size = w * h * 3 / 2;

    /* Copy both planes into a single contiguous QByteArray.
     * QByteArray allocates on the heap and is reference-counted, so the
     * signal delivery (QueuedConnection) takes a cheap ref-counted copy. */
    QByteArray data(nv12_size, Qt::Uninitialized);
    uint8_t *dst = reinterpret_cast<uint8_t *>(data.data());

    /* Copy Y plane */
    std::memcpy(dst, frame->plane0, (size_t)(w * h));

    /* Copy UV plane (NV12 interleaved).
     * If plane1 is NULL (RGBA path), zero-fill the chroma area. */
    if (frame->plane1) {
        std::memcpy(dst + w * h, frame->plane1, (size_t)(w * h / 2));
    } else {
        std::memset(dst + w * h, 0x80, (size_t)(w * h / 2));  /* grey chroma */
    }

    /* Emit the signal — Qt delivers this on the GUI thread because
     * VideoRenderer::submitFrame is connected with Qt::QueuedConnection. */
    emit self->videoFrameReady(data, w, h);
}

/*
 * cAudioCallback — called from the session worker thread each audio buffer.
 *
 * Copies PCM int16 samples into a QByteArray and emits audioSamplesReady.
 * The AudioPlayer receives this signal and feeds the data to the audio
 * backend (ALSA / PulseAudio / PipeWire).
 */
void StreamBackendConnector::cAudioCallback(void                   *user,
                                             const rs_audio_frame_t *frame)
{
    auto *self = static_cast<StreamBackendConnector *>(user);
    if (!self || !frame || !frame->samples || frame->num_samples == 0) return;

    /* Copy PCM samples (int16, interleaved) into a QByteArray */
    int byte_count = static_cast<int>(frame->num_samples) * 2;  /* int16 = 2 bytes */
    QByteArray samples(byte_count, Qt::Uninitialized);
    std::memcpy(samples.data(), frame->samples,
                static_cast<size_t>(byte_count));

    emit self->audioSamplesReady(samples,
                                 static_cast<int>(frame->num_samples),
                                 frame->channels,
                                 frame->sample_rate);
}

/*
 * cStateCallback — called when the session state changes.
 *
 * Converts the C string to a QString and emits connectionStateChanged.
 */
void StreamBackendConnector::cStateCallback(void *user, const char *state) {
    auto *self = static_cast<StreamBackendConnector *>(user);
    if (!self || !state) return;
    emit self->connectionStateChanged(QString::fromUtf8(state));
}

