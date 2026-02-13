/*
 * RootStream KDE Plasma Client - Main Client Wrapper
 * 
 * Wraps the RootStream C API for Qt/QML integration
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

class RootStreamClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString connectionState READ getConnectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString peerHostname READ getPeerHostname NOTIFY peerHostnameChanged)
    
public:
    explicit RootStreamClient(QObject *parent = nullptr);
    ~RootStreamClient();
    
    // Connection management
    Q_INVOKABLE int connectToPeer(const QString &rootstreamCode);
    Q_INVOKABLE int connectToAddress(const QString &hostname, quint16 port);
    Q_INVOKABLE void disconnect();
    
    // Settings/configuration
    Q_INVOKABLE void setVideoCodec(const QString &codec);
    Q_INVOKABLE void setBitrate(quint32 bitrate_bps);
    Q_INVOKABLE void setDisplayMode(const QString &mode);
    Q_INVOKABLE void setAudioDevice(const QString &device);
    Q_INVOKABLE void setInputMode(const QString &mode);
    
    // AI Logging
    Q_INVOKABLE void setAILoggingEnabled(bool enabled);
    Q_INVOKABLE QString getLogOutput();
    
    // Diagnostics
    Q_INVOKABLE QString systemDiagnostics();
    
    // State queries
    bool isConnected() const;
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
    
private:
    rootstream_ctx_t *m_ctx;
    QThread *m_networkThread;
    QTimer *m_eventLoopTimer;
    bool m_connected;
    QString m_connectionState;
    QString m_peerHostname;
    
    void initializeContext();
    void cleanupContext();
};

#endif // ROOTSTREAMCLIENT_H
