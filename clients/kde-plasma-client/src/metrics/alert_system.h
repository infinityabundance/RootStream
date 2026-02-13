#ifndef ALERT_SYSTEM_H
#define ALERT_SYSTEM_H

#include <QObject>
#include "metrics_types.h"

class AlertSystem : public QObject {
    Q_OBJECT
    
public:
    explicit AlertSystem(QObject* parent = nullptr);
    ~AlertSystem();
    
    // Initialize alert system
    bool init();
    
    // Configure thresholds
    void setFPSDropThreshold(uint32_t fps);
    void setLatencyThreshold(uint32_t ms);
    void setAVSyncThreshold(int32_t ms);
    void setThermalThreshold(uint8_t celsius);
    void setPacketLossThreshold(float percent);
    
    // Check metrics and generate alerts
    void checkMetrics(const metrics_snapshot_t& metrics);
    
    // Enable/disable alerts
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    
signals:
    void alertFPSDrop(uint32_t fps);
    void alertHighLatency(uint32_t latency_ms);
    void alertAVSyncDrift(int32_t offset_ms);
    void alertThermalThrottling(const QString& component, uint8_t temp_c);
    void alertHighPacketLoss(float loss_percent);
    
private:
    struct AlertThresholds {
        uint32_t fpsDropThreshold;
        uint32_t latencyThresholdMs;
        int32_t avSyncThresholdMs;
        uint8_t thermalThresholdC;
        float packetLossThresholdPercent;
    } m_thresholds;
    
    bool m_enabled;
    
    // Debouncing to avoid alert spam
    uint64_t m_lastFPSAlert;
    uint64_t m_lastLatencyAlert;
    uint64_t m_lastAVSyncAlert;
    uint64_t m_lastThermalAlert;
    uint64_t m_lastPacketLossAlert;
    static const uint64_t ALERT_DEBOUNCE_MS = 5000; // 5 seconds
};

#endif // ALERT_SYSTEM_H
