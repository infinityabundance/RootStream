#include "metrics_manager.h"
#include <QDebug>

MetricsManager::MetricsManager(QObject* parent)
    : QObject(parent)
    , m_aggregator(nullptr)
    , m_hudRenderer(nullptr)
    , m_logger(nullptr)
    , m_alertSystem(nullptr)
    , m_initialized(false)
    , m_metricsEnabled(true)
    , m_loggingEnabled(false)
{
}

MetricsManager::~MetricsManager() {
    cleanup();
}

bool MetricsManager::init(int window_width, int window_height) {
    if (m_initialized) {
        qWarning() << "MetricsManager already initialized";
        return true;
    }
    
    qDebug() << "Initializing MetricsManager with resolution:" << window_width << "x" << window_height;
    
    // Create and initialize performance aggregator
    m_aggregator = new PerformanceAggregator(this);
    if (!m_aggregator->init()) {
        qWarning() << "Failed to initialize Performance Aggregator";
        cleanup();
        return false;
    }
    
    // Create and initialize HUD renderer
    m_hudRenderer = new HUDRenderer(this);
    if (!m_hudRenderer->init(window_width, window_height)) {
        qWarning() << "Failed to initialize HUD Renderer";
        cleanup();
        return false;
    }
    
    // Create and initialize logger
    m_logger = new PerformanceLogger(this);
    
    // Create and initialize alert system
    m_alertSystem = new AlertSystem(this);
    if (!m_alertSystem->init()) {
        qWarning() << "Failed to initialize Alert System";
        cleanup();
        return false;
    }
    
    // Connect signals
    connect(m_aggregator, &PerformanceAggregator::metricsUpdated,
            this, &MetricsManager::onMetricsUpdated);
    
    connect(m_aggregator, &PerformanceAggregator::fpsDropDetected,
            this, &MetricsManager::fpsDropDetected);
    
    connect(m_aggregator, &PerformanceAggregator::highLatencyDetected,
            this, &MetricsManager::highLatencyDetected);
    
    connect(m_aggregator, &PerformanceAggregator::thermalWarning,
            this, &MetricsManager::thermalWarning);
    
    connect(m_alertSystem, &AlertSystem::alertFPSDrop,
            this, &MetricsManager::fpsDropDetected);
    
    connect(m_alertSystem, &AlertSystem::alertHighLatency,
            this, &MetricsManager::highLatencyDetected);
    
    connect(m_alertSystem, &AlertSystem::alertThermalThrottling,
            this, &MetricsManager::thermalWarning);
    
    m_initialized = true;
    qDebug() << "MetricsManager initialized successfully";
    
    return true;
}

void MetricsManager::cleanup() {
    if (m_logger && m_loggingEnabled) {
        m_logger->finalize();
    }
    
    // Qt will handle deletion via parent relationship
    m_aggregator = nullptr;
    m_hudRenderer = nullptr;
    m_logger = nullptr;
    m_alertSystem = nullptr;
    
    m_initialized = false;
}

void MetricsManager::recordFrame() {
    if (!m_initialized || !m_metricsEnabled) return;
    if (m_aggregator) {
        m_aggregator->recordFrame();
    }
}

void MetricsManager::recordNetworkLatency(uint32_t rtt_ms) {
    if (!m_initialized || !m_metricsEnabled) return;
    if (m_aggregator) {
        m_aggregator->recordNetworkLatency(rtt_ms);
    }
}

void MetricsManager::recordPacketLoss(float loss_percent) {
    if (!m_initialized || !m_metricsEnabled) return;
    if (m_aggregator) {
        m_aggregator->recordPacketLoss(loss_percent);
    }
}

void MetricsManager::recordInputLatency(uint32_t latency_ms) {
    if (!m_initialized || !m_metricsEnabled) return;
    if (m_aggregator) {
        m_aggregator->recordInput(latency_ms);
    }
}

void MetricsManager::recordAVSyncOffset(int32_t offset_ms) {
    if (!m_initialized || !m_metricsEnabled) return;
    if (m_aggregator) {
        m_aggregator->recordAVSyncOffset(offset_ms);
    }
}

void MetricsManager::renderHUD(QPainter* painter) {
    if (!m_initialized || !m_metricsEnabled || !m_hudRenderer) return;
    if (!m_hudRenderer->isHUDVisible()) return;
    
    metrics_snapshot_t snapshot = getLatestSnapshot();
    m_hudRenderer->renderHUD(snapshot, painter);
}

void MetricsManager::setHUDVisible(bool visible) {
    if (m_hudRenderer) {
        m_hudRenderer->setHUDVisible(visible);
        qDebug() << "HUD visibility set to:" << visible;
    }
}

void MetricsManager::setMetricsEnabled(bool enabled) {
    m_metricsEnabled = enabled;
    if (m_aggregator) {
        m_aggregator->setEnabled(enabled);
    }
    qDebug() << "Metrics enabled:" << enabled;
}

void MetricsManager::setLoggingEnabled(bool enabled, const QString& filename) {
    if (!m_logger) return;
    
    if (enabled && !filename.isEmpty()) {
        if (m_logger->init(filename)) {
            m_loggingEnabled = true;
            qDebug() << "Performance logging enabled to:" << filename;
        } else {
            qWarning() << "Failed to enable performance logging";
        }
    } else if (!enabled && m_loggingEnabled) {
        m_logger->finalize();
        m_loggingEnabled = false;
        qDebug() << "Performance logging disabled";
    }
}

void MetricsManager::setAlertsEnabled(bool enabled) {
    if (m_alertSystem) {
        m_alertSystem->setEnabled(enabled);
        qDebug() << "Alerts enabled:" << enabled;
    }
}

bool MetricsManager::isHUDVisible() const {
    return m_hudRenderer ? m_hudRenderer->isHUDVisible() : false;
}

bool MetricsManager::isMetricsEnabled() const {
    return m_metricsEnabled;
}

metrics_snapshot_t MetricsManager::getLatestSnapshot() {
    if (m_aggregator) {
        return m_aggregator->getLatestSnapshot();
    }
    
    metrics_snapshot_t empty;
    memset(&empty, 0, sizeof(empty));
    return empty;
}

void MetricsManager::onMetricsUpdated(const metrics_snapshot_t& snapshot) {
    // Log to CSV if enabled
    if (m_loggingEnabled && m_logger) {
        m_logger->logSnapshotCSV(snapshot);
        m_logger->logSnapshotJSON(snapshot);
    }
    
    // Check alerts
    if (m_alertSystem) {
        m_alertSystem->checkMetrics(snapshot);
    }
    
    // Forward the signal
    emit metricsUpdated(snapshot);
}
