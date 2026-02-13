#ifndef METRICS_MANAGER_H
#define METRICS_MANAGER_H

#include <QObject>
#include "performance_aggregator.h"
#include "hud_renderer.h"
#include "performance_logger.h"
#include "alert_system.h"

class MetricsManager : public QObject {
    Q_OBJECT
    
public:
    explicit MetricsManager(QObject* parent = nullptr);
    ~MetricsManager();
    
    // Initialize all metrics subsystems
    bool init(int window_width = 1920, int window_height = 1080);
    
    // Cleanup
    void cleanup();
    
    // Recording interface (to be called by other components)
    void recordFrame();
    void recordNetworkLatency(uint32_t rtt_ms);
    void recordPacketLoss(float loss_percent);
    void recordInputLatency(uint32_t latency_ms);
    void recordAVSyncOffset(int32_t offset_ms);
    
    // Rendering interface
    void renderHUD(QPainter* painter);
    
    // Configuration
    void setHUDVisible(bool visible);
    void setMetricsEnabled(bool enabled);
    void setLoggingEnabled(bool enabled, const QString& filename = "");
    void setAlertsEnabled(bool enabled);
    
    // Query methods
    bool isHUDVisible() const;
    bool isMetricsEnabled() const;
    metrics_snapshot_t getLatestSnapshot();
    
    // Getters for subsystems
    PerformanceAggregator* getAggregator() { return m_aggregator; }
    HUDRenderer* getHUDRenderer() { return m_hudRenderer; }
    PerformanceLogger* getLogger() { return m_logger; }
    AlertSystem* getAlertSystem() { return m_alertSystem; }
    
signals:
    void metricsUpdated(const metrics_snapshot_t& snapshot);
    void fpsDropDetected(uint32_t fps);
    void highLatencyDetected(uint32_t latency_ms);
    void thermalWarning(const QString& component, uint8_t temp_c);
    
private slots:
    void onMetricsUpdated(const metrics_snapshot_t& snapshot);
    
private:
    PerformanceAggregator* m_aggregator;
    HUDRenderer* m_hudRenderer;
    PerformanceLogger* m_logger;
    AlertSystem* m_alertSystem;
    
    bool m_initialized;
    bool m_metricsEnabled;
    bool m_loggingEnabled;
};

#endif // METRICS_MANAGER_H
