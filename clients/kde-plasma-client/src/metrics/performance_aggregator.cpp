#include "performance_aggregator.h"
#include <QDebug>
#include <cstring>
#include <ctime>
#include <algorithm>

PerformanceAggregator::PerformanceAggregator(QObject* parent)
    : QObject(parent)
    , m_fpsCounter(nullptr)
    , m_cpuMonitor(nullptr)
    , m_memoryMonitor(nullptr)
    , m_gpuMonitor(nullptr)
    , m_snapshotIndex(0)
    , m_updateTimer(nullptr)
    , m_enabled(false)
    , m_currentRTT(0)
    , m_currentPacketLoss(0.0f)
    , m_lastInputLatency(0)
    , m_lastAVSyncOffset(0)
{
    memset(m_snapshots, 0, sizeof(m_snapshots));
}

PerformanceAggregator::~PerformanceAggregator() {
    cleanup();
}

bool PerformanceAggregator::init() {
    qDebug() << "Initializing Performance Aggregator";
    
    // Initialize all monitors
    m_fpsCounter = frame_rate_counter_init();
    if (!m_fpsCounter) {
        qWarning() << "Failed to initialize FPS counter";
        return false;
    }
    
    m_cpuMonitor = cpu_monitor_init();
    if (!m_cpuMonitor) {
        qWarning() << "Failed to initialize CPU monitor";
        cleanup();
        return false;
    }
    
    m_memoryMonitor = memory_monitor_init();
    if (!m_memoryMonitor) {
        qWarning() << "Failed to initialize memory monitor";
        cleanup();
        return false;
    }
    
    m_gpuMonitor = gpu_monitor_init();
    if (!m_gpuMonitor) {
        qWarning() << "Failed to initialize GPU monitor";
        cleanup();
        return false;
    }
    
    // Create update timer (update every 1000ms)
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &PerformanceAggregator::updateMetrics);
    m_updateTimer->setInterval(1000);
    
    m_enabled = true;
    m_updateTimer->start();
    
    qDebug() << "Performance Aggregator initialized successfully";
    return true;
}

void PerformanceAggregator::cleanup() {
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    
    if (m_fpsCounter) {
        frame_rate_counter_cleanup(m_fpsCounter);
        m_fpsCounter = nullptr;
    }
    
    if (m_cpuMonitor) {
        cpu_monitor_cleanup(m_cpuMonitor);
        m_cpuMonitor = nullptr;
    }
    
    if (m_memoryMonitor) {
        memory_monitor_cleanup(m_memoryMonitor);
        m_memoryMonitor = nullptr;
    }
    
    if (m_gpuMonitor) {
        gpu_monitor_cleanup(m_gpuMonitor);
        m_gpuMonitor = nullptr;
    }
}

uint64_t PerformanceAggregator::getTimestampUs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

void PerformanceAggregator::recordFrame() {
    if (!m_enabled || !m_fpsCounter) return;
    frame_rate_counter_record_frame(m_fpsCounter);
}

void PerformanceAggregator::recordInput(uint64_t latency_ms) {
    if (!m_enabled) return;
    m_lastInputLatency = static_cast<uint32_t>(latency_ms);
}

void PerformanceAggregator::recordNetworkLatency(uint32_t rtt_ms) {
    if (!m_enabled) return;
    m_currentRTT = rtt_ms;
}

void PerformanceAggregator::recordAVSyncOffset(int32_t offset_ms) {
    if (!m_enabled) return;
    m_lastAVSyncOffset = offset_ms;
}

void PerformanceAggregator::recordPacketLoss(float loss_percent) {
    if (!m_enabled) return;
    m_currentPacketLoss = loss_percent;
}

void PerformanceAggregator::updateMetrics() {
    if (!m_enabled) return;
    
    // Update system monitors
    if (m_cpuMonitor) cpu_monitor_update(m_cpuMonitor);
    if (m_memoryMonitor) memory_monitor_update(m_memoryMonitor);
    if (m_gpuMonitor) gpu_monitor_update(m_gpuMonitor);
    
    // Create snapshot
    metrics_snapshot_t snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.timestamp_us = getTimestampUs();
    
    // Collect FPS metrics
    if (m_fpsCounter) {
        frame_rate_counter_get_stats(m_fpsCounter, &snapshot.fps);
    }
    
    // Collect network metrics
    snapshot.network.rtt_ms = m_currentRTT;
    snapshot.network.packet_loss_percent = m_currentPacketLoss;
    snapshot.network.avg_rtt_ms = m_currentRTT; // Simplified for now
    snapshot.network.min_rtt_ms = m_currentRTT;
    snapshot.network.max_rtt_ms = m_currentRTT;
    
    // Collect input metrics
    snapshot.input.input_latency_ms = m_lastInputLatency;
    snapshot.input.avg_input_latency_ms = m_lastInputLatency;
    
    // Collect A/V sync metrics
    snapshot.av_sync.av_sync_offset_ms = m_lastAVSyncOffset;
    
    // Collect system metrics
    if (m_cpuMonitor) {
        cpu_monitor_get_stats(m_cpuMonitor, &snapshot.cpu);
    }
    if (m_memoryMonitor) {
        memory_monitor_get_stats(m_memoryMonitor, &snapshot.memory);
    }
    if (m_gpuMonitor) {
        gpu_monitor_get_stats(m_gpuMonitor, &snapshot.gpu);
    }
    
    // Store snapshot
    m_snapshots[m_snapshotIndex] = snapshot;
    m_snapshotIndex = (m_snapshotIndex + 1) % METRICS_HISTORY_SIZE;
    
    // Emit signal
    emit metricsUpdated(snapshot);
    
    // Check for anomalies
    if (detectFPSDrop()) {
        emit fpsDropDetected(snapshot.fps.fps);
    }
    
    if (detectHighLatency()) {
        emit highLatencyDetected(snapshot.network.rtt_ms);
    }
    
    if (detectThermalThrottling()) {
        if (snapshot.gpu.thermal_throttling) {
            emit thermalWarning("GPU", snapshot.gpu.gpu_temp_celsius);
        }
        if (snapshot.cpu.thermal_throttling) {
            emit thermalWarning("CPU", snapshot.cpu.cpu_temp_celsius);
        }
    }
}

metrics_snapshot_t PerformanceAggregator::getLatestSnapshot() {
    if (m_snapshotIndex == 0) {
        return m_snapshots[METRICS_HISTORY_SIZE - 1];
    }
    return m_snapshots[m_snapshotIndex - 1];
}

percentile_stats_t PerformanceAggregator::getFPSPercentiles() {
    percentile_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    
    // Collect FPS samples
    std::vector<uint32_t> fps_samples;
    for (uint32_t i = 0; i < METRICS_HISTORY_SIZE; i++) {
        if (m_snapshots[i].timestamp_us > 0) {
            fps_samples.push_back(m_snapshots[i].fps.fps);
        }
    }
    
    if (fps_samples.empty()) return stats;
    
    std::sort(fps_samples.begin(), fps_samples.end());
    
    size_t n = fps_samples.size();
    stats.p50 = fps_samples[n * 50 / 100];
    stats.p75 = fps_samples[n * 75 / 100];
    stats.p95 = fps_samples[n * 95 / 100];
    stats.p99 = fps_samples[n * 99 / 100];
    
    return stats;
}

bool PerformanceAggregator::detectFPSDrop() {
    if (m_snapshotIndex == 0) return false;
    
    uint32_t current_idx = (m_snapshotIndex + METRICS_HISTORY_SIZE - 1) % METRICS_HISTORY_SIZE;
    uint32_t fps = m_snapshots[current_idx].fps.fps;
    
    // Detect if FPS drops below 30
    return fps > 0 && fps < 30;
}

bool PerformanceAggregator::detectHighLatency() {
    if (m_snapshotIndex == 0) return false;
    
    uint32_t current_idx = (m_snapshotIndex + METRICS_HISTORY_SIZE - 1) % METRICS_HISTORY_SIZE;
    uint32_t rtt = m_snapshots[current_idx].network.rtt_ms;
    
    // Detect if RTT > 100ms
    return rtt > 100;
}

bool PerformanceAggregator::detectThermalThrottling() {
    if (m_snapshotIndex == 0) return false;
    
    uint32_t current_idx = (m_snapshotIndex + METRICS_HISTORY_SIZE - 1) % METRICS_HISTORY_SIZE;
    return m_snapshots[current_idx].gpu.thermal_throttling || 
           m_snapshots[current_idx].cpu.thermal_throttling;
}

void PerformanceAggregator::setEnabled(bool enabled) {
    m_enabled = enabled;
    if (m_updateTimer) {
        if (enabled) {
            m_updateTimer->start();
        } else {
            m_updateTimer->stop();
        }
    }
}

void PerformanceAggregator::onVideoFrameRendered() {
    recordFrame();
}

void PerformanceAggregator::onInputProcessed() {
    // Input latency would be calculated elsewhere and passed via recordInput()
}

void PerformanceAggregator::onNetworkPacketReceived() {
    // Network latency would be calculated elsewhere and passed via recordNetworkLatency()
}
