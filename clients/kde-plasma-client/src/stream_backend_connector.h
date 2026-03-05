/*
 * stream_backend_connector.h — Bridge between rs_client_session and Qt
 *
 * PHASE-95.3 REWRITE
 * ------------------
 * This file previously used the local network_client.h (a duplicate of the
 * core protocol logic that was never connected to the real backend).
 *
 * It now uses rs_client_session_t (from include/rootstream_client_session.h),
 * which IS the real backend.  This eliminates the gap described in:
 *   "docs/IMPLEMENTATION_STATUS.md references StreamBackendConnector.cpp
 *    which does not exist anywhere … and the KDE client has no implemented bridge."
 *
 * OVERVIEW
 * --------
 * StreamBackendConnector is a QObject that:
 *   1. Owns the rs_client_session_t lifecycle.
 *   2. Runs the session receive/decode loop on a QThread (not the UI thread).
 *   3. Converts C callback frames (rs_video_frame_t, rs_audio_frame_t) to
 *      Qt signals so VideoRenderer and AudioPlayer can consume them safely
 *      across thread boundaries.
 *
 * THREADING
 * ---------
 *   GUI thread:     creates StreamBackendConnector, calls connectToHost/disconnect
 *   Session thread: rs_client_session_run() blocks here; C callbacks fire here
 *   Render thread:  VideoRenderer::synchronize() and render() run here
 *
 * The C callbacks copy frame data and emit Qt signals (QueuedConnection),
 * so video/audio handling always happens on the correct thread.
 *
 * FRAME LIFETIME
 * --------------
 * rs_video_frame_t pointers are valid ONLY for the callback duration.
 * We memcpy the NV12 data into a QByteArray immediately in cVideoCallback(),
 * before the callback returns.  The signal carries the copy.
 */

#ifndef STREAM_BACKEND_CONNECTOR_H
#define STREAM_BACKEND_CONNECTOR_H

#include <QObject>
#include <QThread>
#include <QByteArray>
#include <QString>

extern "C" {
#include "rootstream_client_session.h"
}

class StreamBackendConnector : public QObject
{
    Q_OBJECT

public:
    explicit StreamBackendConnector(QObject *parent = nullptr);
    ~StreamBackendConnector() override;

    /**
     * connectToHost — create a session and start streaming on a worker thread.
     *
     * @param host  Peer hostname or IP address
     * @param port  Peer port number
     * @param code  Optional pairing code (may be empty)
     */
    void connectToHost(const QString &host, int port, const QString &code = {});

    /**
     * disconnect — stop the session and join the worker thread.
     *
     * Safe to call if not connected.  Blocks for at most one recv poll cycle.
     */
    void disconnect();

    /** True while the session thread is running */
    bool isConnected() const;

    /** Returns the decoder backend name after streaming starts */
    QString decoderName() const;

signals:
    /**
     * videoFrameReady — emitted (QueuedConnection) when a decoded frame is ready.
     *
     * Connect to VideoRenderer::submitFrame with Qt::QueuedConnection:
     *   connect(connector, &StreamBackendConnector::videoFrameReady,
     *           renderer,  &VideoRenderer::submitFrame,
     *           Qt::QueuedConnection);
     *
     * @param nv12_data  NV12 frame: Y plane (width×height) + UV plane (width×height/2)
     * @param width      Frame width in pixels
     * @param height     Frame height in pixels
     */
    void videoFrameReady(QByteArray nv12_data, int width, int height);

    /**
     * audioSamplesReady — emitted when a decoded audio buffer is available.
     *
     * @param samples      PCM int16 samples (QByteArray of int16 values)
     * @param num_samples  Total samples (frames × channels)
     * @param channels     Number of audio channels
     * @param sample_rate  Sample rate in Hz (e.g. 48000)
     */
    void audioSamplesReady(QByteArray samples, int num_samples,
                           int channels, int sample_rate);

    /** Emitted when connection state changes ("connecting", "connected", "disconnected", etc.) */
    void connectionStateChanged(QString state);

    /** Emitted once the session thread has fully exited */
    void sessionStopped();

private:
    /* Static C callback trampolines — registered with rs_client_session.
     * 'user' points to the StreamBackendConnector instance. */
    static void cVideoCallback(void *user, const rs_video_frame_t *frame);
    static void cAudioCallback(void *user, const rs_audio_frame_t *frame);
    static void cStateCallback(void *user, const char *state);

    rs_client_session_t *session_ = nullptr;  /**< Owned session handle */
    QThread             *thread_  = nullptr;  /**< Session worker thread */

    /* Connection parameters — stored for reconnect support */
    QString host_;
    int     port_ = 0;
    QString code_;
};

#endif /* STREAM_BACKEND_CONNECTOR_H */

