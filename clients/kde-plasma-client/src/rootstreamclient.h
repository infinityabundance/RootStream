/*
 * RootStream KDE Plasma Client - Main Client Wrapper
 *
 * Wraps the RootStream C API for Qt/QML integration.
 *
 * PHASE-96 CHANGES
 * ----------------
 * Added StreamBackendConnector member.  connectToPeer() / connectToAddress()
 * now delegate the actual streaming session to StreamBackendConnector, which:
 *   - Creates an rs_client_session_t (real rootstream_core backend)
 *   - Runs the receive/decode loop on a dedicated QThread
 *   - Emits videoFrameReady → VideoRenderer::submitFrame (QueuedConnection)
 *   - Emits audioSamplesReady → AudioPlayer (QueuedConnection)
 *
 * The previous approach called rootstream_connect_to_peer() which existed
 * as a stub in the context and didn't start a session.  Now the full
 * decode/render pipeline is wired end-to-end.
 */

#ifndef ROOTSTREAMCLIENT_H
#define ROOTSTREAMCLIENT_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QTimer>

extern "C" {
#include "rootstream.h"
}

/* PHASE-96: StreamBackendConnector — owns the rs_client_session lifecycle */
#include "stream_backend_connector.h"

/* Forward declare VideoRenderer to avoid circular include */
class VideoRenderer;

class RootStreamClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString connectionState READ getConnectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString peerHostname READ getPeerHostname NOTIFY peerHostnameChanged)

public:
    explicit RootStreamClient(QObject *parent = nullptr);
    ~RootStreamClient();

    /* ── Connection management ─────────────────────────────────────── */
    Q_INVOKABLE int connectToPeer(const QString &rootstreamCode);
    Q_INVOKABLE int connectToAddress(const QString &hostname, quint16 port);
    Q_INVOKABLE void disconnect();

    /* ── Video renderer wiring (PHASE-96) ──────────────────────────── */
    /**
     * setVideoRenderer — wire the StreamBackendConnector to a VideoRenderer.
     *
     * Call this once after both objects are created (e.g. in main.cpp or QML
     * onCompleted).  After this call, decoded frames will flow to the renderer
     * automatically when a connection is active.
     *
     * @param renderer  The VideoRenderer QML item to receive frames
     */
    Q_INVOKABLE void setVideoRenderer(VideoRenderer *renderer);

    /* ── Settings/configuration ────────────────────────────────────── */
    Q_INVOKABLE void setVideoCodec(const QString &codec);
    Q_INVOKABLE void setBitrate(quint32 bitrate_bps);
    Q_INVOKABLE void setDisplayMode(const QString &mode);
    Q_INVOKABLE void setAudioDevice(const QString &device);
    Q_INVOKABLE void setInputMode(const QString &mode);

    /* ── AI Logging ────────────────────────────────────────────────── */
    Q_INVOKABLE void setAILoggingEnabled(bool enabled);
    Q_INVOKABLE QString getLogOutput();

    /* ── Diagnostics ───────────────────────────────────────────────── */
    Q_INVOKABLE QString systemDiagnostics();

    /* ── State queries ─────────────────────────────────────────────── */
    bool    isConnected() const;
    QString getConnectionState() const;
    QString getPeerHostname() const;

signals:
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    void videoFrameReceived(quint64 timestamp);
    void audioSamplesReceived(quint32 sampleCount);
    void peerDiscovered(const QString &code, const QString &hostname);
    void peerLost(const QString &code);
    void statusUpdated(const QString &status);
    void performanceMetrics(double fps, quint32 latency_ms, const QString &resolution);
    void connectedChanged();
    void connectionStateChanged();
    void peerHostnameChanged();

private slots:
    void processEvents();
    void onSessionStateChanged(const QString &state);
    void onSessionStopped();

private:
    rootstream_ctx_t     *m_ctx;
    QThread              *m_networkThread;
    QTimer               *m_eventLoopTimer;
    bool                  m_connected;
    QString               m_connectionState;
    QString               m_peerHostname;

    /* PHASE-96: The connector that owns the real streaming session */
    StreamBackendConnector *m_connector;

    /* The video renderer to wire frames to (set via setVideoRenderer()) */
    VideoRenderer          *m_renderer;

    void initializeContext();
    void cleanupContext();
};

#endif /* ROOTSTREAMCLIENT_H */
