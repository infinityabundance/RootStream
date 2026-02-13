#ifndef PERFORMANCE_AGGREGATOR_H
#define PERFORMANCE_AGGREGATOR_H

#include <QObject>
#include <QTimer>
#include "metrics_types.h"

extern "C" {
#include "frame_rate_counter.h"
#include "cpu_monitor.h"
#include "memory_monitor.h"
#include "gpu_monitor.h"
}

class PerformanceAggregator : public QObject {
    Q_OBJECT
    
public:
    explicit PerformanceAggregator(QObject* parent = nullptr);
    ~PerformanceAggregator();
    
    // Initialize metrics system
    bool init();
    
    // Record events (called from various subsystems)
    void recordFrame();
    void recordInput(uint64_t latency_ms);
    void recordNetworkLatency(uint32_t rtt_ms);
    void recordAVSyncOffset(int32_t offset_ms);
    void recordPacketLoss(float loss_percent);
    
    // Query aggregated metrics
    metrics_snapshot_t getLatestSnapshot();
    percentile_stats_t getFPSPercentiles();
    
    // Anomaly detection
    bool detectFPSDrop();
    bool detectHighLatency();
    bool detectThermalThrottling();
    
    // Enable/disable metrics collection
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    
signals:
    void metricsUpdated(const metrics_snapshot_t& snapshot);
    void fpsDropDetected(uint32_t fps);
    void highLatencyDetected(uint32_t latency_ms);
    void thermalWarning(const QString& component, uint8_t temp_c);
    
public slots:
    void onVideoFrameRendered();
    void onInputProcessed();
    void onNetworkPacketReceived();
    
private slots:
    void updateMetrics();
    
private:
    void cleanup();
    uint64_t getTimestampUs();
    
    frame_rate_counter_t* m_fpsCounter;
    cpu_monitor_t* m_cpuMonitor;
    memory_monitor_t* m_memoryMonitor;
    gpu_monitor_t* m_gpuMonitor;
    
    metrics_snapshot_t m_snapshots[METRICS_HISTORY_SIZE];
    uint32_t m_snapshotIndex;
    
    QTimer* m_updateTimer;
    bool m_enabled;
    
    // Cached metrics
    uint32_t m_currentRTT;
    float m_currentPacketLoss;
    uint32_t m_lastInputLatency;
    int32_t m_lastAVSyncOffset;
};

#endif // PERFORMANCE_AGGREGATOR_H
