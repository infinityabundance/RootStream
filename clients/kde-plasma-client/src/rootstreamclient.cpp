/*
 * RootStream KDE Plasma Client - Main Client Wrapper Implementation
 */

#include "rootstreamclient.h"
#include <QDebug>
#include <QCoreApplication>

extern "C" {
#include "ai_logging.h"
}

RootStreamClient::RootStreamClient(QObject *parent)
    : QObject(parent)
    , m_ctx(nullptr)
    , m_networkThread(nullptr)
    , m_eventLoopTimer(nullptr)
    , m_connected(false)
    , m_connectionState("Disconnected")
{
    initializeContext();
    
    // Create event loop timer for processing network events
    m_eventLoopTimer = new QTimer(this);
    connect(m_eventLoopTimer, &QTimer::timeout, this, &RootStreamClient::processEvents);
    m_eventLoopTimer->start(16); // ~60Hz for low latency
}

RootStreamClient::~RootStreamClient()
{
    cleanupContext();
}

void RootStreamClient::initializeContext()
{
    m_ctx = (rootstream_ctx_t*)calloc(1, sizeof(rootstream_ctx_t));
    if (!m_ctx) {
        qCritical() << "Failed to allocate RootStream context";
        return;
    }
    
    // Initialize crypto/identity
    if (rootstream_crypto_init(m_ctx) < 0) {
        qCritical() << "Failed to initialize crypto";
        free(m_ctx);
        m_ctx = nullptr;
        return;
    }
    
    // Initialize network
    if (rootstream_net_init(m_ctx, 9876) < 0) {
        qCritical() << "Failed to initialize network";
        free(m_ctx);
        m_ctx = nullptr;
        return;
    }
    
    // Initialize decoder
    if (rootstream_decoder_init(m_ctx) < 0) {
        qWarning() << "Failed to initialize hardware decoder, using software fallback";
    }
    
    // Initialize audio decoder
    if (rootstream_opus_decoder_init(m_ctx) < 0) {
        qWarning() << "Failed to initialize Opus decoder";
    }
    
    // Initialize audio playback
    if (audio_playback_init(m_ctx) < 0) {
        qWarning() << "Failed to initialize audio playback";
    }
    
    // Set default settings
    m_ctx->settings.bitrate_bps = 10000000; // 10 Mbps default
    m_ctx->encoder.codec = CODEC_H264;
    m_ctx->settings.audio_enabled = true;
    
    qInfo() << "RootStream client initialized successfully";
}

void RootStreamClient::cleanupContext()
{
    if (m_eventLoopTimer) {
        m_eventLoopTimer->stop();
    }
    
    if (m_ctx) {
        disconnect();
        
        audio_playback_cleanup(m_ctx);
        rootstream_opus_cleanup(m_ctx);
        rootstream_decoder_cleanup(m_ctx);
        rootstream_net_cleanup(m_ctx);
        rootstream_crypto_cleanup(m_ctx);
        
        free(m_ctx);
        m_ctx = nullptr;
    }
}

int RootStreamClient::connectToPeer(const QString &rootstreamCode)
{
    if (!m_ctx) {
        emit connectionError("Client not initialized");
        return -1;
    }
    
    if (m_connected) {
        emit connectionError("Already connected");
        return -1;
    }
    
    qInfo() << "Connecting to peer:" << rootstreamCode;
    
    int ret = rootstream_connect_to_peer(m_ctx, rootstreamCode.toUtf8().constData());
    if (ret < 0) {
        emit connectionError("Failed to connect to peer");
        return -1;
    }
    
    m_connected = true;
    m_connectionState = "Connected";
    m_peerHostname = rootstreamCode;
    
    emit connected();
    emit connectedChanged();
    emit connectionStateChanged();
    emit peerHostnameChanged();
    emit statusUpdated("Connected to " + rootstreamCode);
    
    return 0;
}

int RootStreamClient::connectToAddress(const QString &hostname, quint16 port)
{
    if (!m_ctx) {
        emit connectionError("Client not initialized");
        return -1;
    }
    
    // For now, we need to use the RootStream code format
    // This is a simplified version - in production, you'd need proper address resolution
    QString code = hostname + ":" + QString::number(port);
    return connectToPeer(code);
}

void RootStreamClient::disconnect()
{
    if (!m_ctx || !m_connected) {
        return;
    }
    
    qInfo() << "Disconnecting from peer";
    
    // Clean up all peers
    for (int i = 0; i < m_ctx->num_peers; i++) {
        rootstream_remove_peer(m_ctx, &m_ctx->peers[i]);
    }
    
    m_connected = false;
    m_connectionState = "Disconnected";
    m_peerHostname.clear();
    
    emit disconnected();
    emit connectedChanged();
    emit connectionStateChanged();
    emit peerHostnameChanged();
    emit statusUpdated("Disconnected");
}

void RootStreamClient::setVideoCodec(const QString &codec)
{
    if (!m_ctx) return;
    
    if (codec == "h264") {
        m_ctx->encoder.codec = CODEC_H264;
    } else if (codec == "h265" || codec == "hevc") {
        m_ctx->encoder.codec = CODEC_H265;
    } else if (codec == "vp9") {
        m_ctx->encoder.codec = CODEC_VP9;
    } else if (codec == "vp8") {
        m_ctx->encoder.codec = CODEC_VP8;
    } else {
        qWarning() << "Unknown codec:" << codec;
        return;
    }
    
    qInfo() << "Set video codec to:" << codec;
}

void RootStreamClient::setBitrate(quint32 bitrate_bps)
{
    if (!m_ctx) return;
    
    m_ctx->settings.bitrate_bps = bitrate_bps;
    qInfo() << "Set bitrate to:" << bitrate_bps << "bps";
}

void RootStreamClient::setDisplayMode(const QString &mode)
{
    qInfo() << "Set display mode to:" << mode;
    // This will be handled by the QML UI for fullscreen/windowed mode
}

void RootStreamClient::setAudioDevice(const QString &device)
{
    qInfo() << "Set audio device to:" << device;
    // Audio device selection would be implemented here
}

void RootStreamClient::setInputMode(const QString &mode)
{
    qInfo() << "Set input mode to:" << mode;
    // Input mode would be configured here
}

void RootStreamClient::setAILoggingEnabled(bool enabled)
{
#ifdef ENABLE_AI_LOGGING
    if (m_ctx) {
        ai_logging_set_enabled(m_ctx, enabled);
        qInfo() << "AI logging" << (enabled ? "enabled" : "disabled");
    }
#else
    Q_UNUSED(enabled);
    qWarning() << "AI logging support not compiled in";
#endif
}

QString RootStreamClient::getLogOutput()
{
    // This would retrieve structured logs from the AI logging system
    return QString("Log output not yet implemented");
}

QString RootStreamClient::systemDiagnostics()
{
    if (!m_ctx) {
        return "Client not initialized";
    }
    
    // Call RootStream diagnostics
    QString diag;
    diag += "RootStream KDE Client\n";
    diag += "Version: 1.0.0\n";
    diag += QString("Connected: %1\n").arg(m_connected ? "Yes" : "No");
    diag += QString("Connection State: %1\n").arg(m_connectionState);
    
    if (m_connected && !m_peerHostname.isEmpty()) {
        diag += QString("Peer: %1\n").arg(m_peerHostname);
    }
    
    diag += QString("Codec: %1\n").arg(
        m_ctx->encoder.codec == CODEC_H264 ? "H.264" :
        m_ctx->encoder.codec == CODEC_H265 ? "H.265" :
        m_ctx->encoder.codec == CODEC_VP9 ? "VP9" :
        m_ctx->encoder.codec == CODEC_VP8 ? "VP8" : "Unknown"
    );
    diag += QString("Bitrate: %1 Mbps\n").arg(m_ctx->settings.bitrate_bps / 1000000.0);
    
    return diag;
}

bool RootStreamClient::isConnected() const
{
    return m_connected;
}

QString RootStreamClient::getConnectionState() const
{
    return m_connectionState;
}

QString RootStreamClient::getPeerHostname() const
{
    return m_peerHostname;
}

void RootStreamClient::processEvents()
{
    if (!m_ctx || !m_connected) {
        return;
    }
    
    // Process incoming network packets (non-blocking)
    int ret = rootstream_net_recv(m_ctx, 0);
    if (ret > 0) {
        // Packets received - emit signals as needed
        emit videoFrameReceived(QDateTime::currentMSecsSinceEpoch());
    } else if (ret < 0) {
        // Error occurred
        qWarning() << "Network receive error";
        // Don't disconnect immediately - might just be a temporary issue
    }
    
    // Network tick for keepalives, reconnection, etc.
    rootstream_net_tick(m_ctx);
}
