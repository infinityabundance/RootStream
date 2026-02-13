#include "alert_system.h"
#include <QDebug>
#include <QDateTime>

AlertSystem::AlertSystem(QObject* parent)
    : QObject(parent)
    , m_enabled(false)
    , m_lastFPSAlert(0)
    , m_lastLatencyAlert(0)
    , m_lastAVSyncAlert(0)
    , m_lastThermalAlert(0)
    , m_lastPacketLossAlert(0)
{
    // Default thresholds
    m_thresholds.fpsDropThreshold = 30;
    m_thresholds.latencyThresholdMs = 100;
    m_thresholds.avSyncThresholdMs = 50;
    m_thresholds.thermalThresholdC = 85;
    m_thresholds.packetLossThresholdPercent = 5.0f;
}

AlertSystem::~AlertSystem() {
}

bool AlertSystem::init() {
    m_enabled = true;
    qDebug() << "Alert System initialized with thresholds:"
             << "FPS:" << m_thresholds.fpsDropThreshold
             << "Latency:" << m_thresholds.latencyThresholdMs << "ms"
             << "A/V Sync:" << m_thresholds.avSyncThresholdMs << "ms"
             << "Thermal:" << m_thresholds.thermalThresholdC << "°C"
             << "Packet Loss:" << m_thresholds.packetLossThresholdPercent << "%";
    return true;
}

void AlertSystem::setFPSDropThreshold(uint32_t fps) {
    m_thresholds.fpsDropThreshold = fps;
    qDebug() << "FPS drop threshold set to:" << fps;
}

void AlertSystem::setLatencyThreshold(uint32_t ms) {
    m_thresholds.latencyThresholdMs = ms;
    qDebug() << "Latency threshold set to:" << ms << "ms";
}

void AlertSystem::setAVSyncThreshold(int32_t ms) {
    m_thresholds.avSyncThresholdMs = ms;
    qDebug() << "A/V sync threshold set to:" << ms << "ms";
}

void AlertSystem::setThermalThreshold(uint8_t celsius) {
    m_thresholds.thermalThresholdC = celsius;
    qDebug() << "Thermal threshold set to:" << celsius << "°C";
}

void AlertSystem::setPacketLossThreshold(float percent) {
    m_thresholds.packetLossThresholdPercent = percent;
    qDebug() << "Packet loss threshold set to:" << percent << "%";
}

void AlertSystem::checkMetrics(const metrics_snapshot_t& metrics) {
    if (!m_enabled) return;
    
    uint64_t now = QDateTime::currentMSecsSinceEpoch();
    
    // Check FPS drop
    if (metrics.fps.fps > 0 && metrics.fps.fps < m_thresholds.fpsDropThreshold) {
        if (now - m_lastFPSAlert > ALERT_DEBOUNCE_MS) {
            emit alertFPSDrop(metrics.fps.fps);
            m_lastFPSAlert = now;
            qWarning() << "ALERT: FPS dropped to" << metrics.fps.fps;
        }
    }
    
    // Check high latency
    if (metrics.network.rtt_ms > m_thresholds.latencyThresholdMs) {
        if (now - m_lastLatencyAlert > ALERT_DEBOUNCE_MS) {
            emit alertHighLatency(metrics.network.rtt_ms);
            m_lastLatencyAlert = now;
            qWarning() << "ALERT: High latency detected:" << metrics.network.rtt_ms << "ms";
        }
    }
    
    // Check A/V sync drift
    int32_t abs_offset = qAbs(metrics.av_sync.av_sync_offset_ms);
    if (abs_offset > m_thresholds.avSyncThresholdMs) {
        if (now - m_lastAVSyncAlert > ALERT_DEBOUNCE_MS) {
            emit alertAVSyncDrift(metrics.av_sync.av_sync_offset_ms);
            m_lastAVSyncAlert = now;
            qWarning() << "ALERT: A/V sync drift detected:" << metrics.av_sync.av_sync_offset_ms << "ms";
        }
    }
    
    // Check thermal throttling
    if (metrics.gpu.thermal_throttling || metrics.cpu.thermal_throttling) {
        if (now - m_lastThermalAlert > ALERT_DEBOUNCE_MS) {
            if (metrics.gpu.thermal_throttling) {
                emit alertThermalThrottling("GPU", metrics.gpu.gpu_temp_celsius);
                qWarning() << "ALERT: GPU thermal throttling at" << metrics.gpu.gpu_temp_celsius << "°C";
            }
            if (metrics.cpu.thermal_throttling) {
                emit alertThermalThrottling("CPU", metrics.cpu.cpu_temp_celsius);
                qWarning() << "ALERT: CPU thermal throttling at" << metrics.cpu.cpu_temp_celsius << "°C";
            }
            m_lastThermalAlert = now;
        }
    }
    
    // Check high packet loss
    if (metrics.network.packet_loss_percent > m_thresholds.packetLossThresholdPercent) {
        if (now - m_lastPacketLossAlert > ALERT_DEBOUNCE_MS) {
            emit alertHighPacketLoss(metrics.network.packet_loss_percent);
            m_lastPacketLossAlert = now;
            qWarning() << "ALERT: High packet loss:" << metrics.network.packet_loss_percent << "%";
        }
    }
}

void AlertSystem::setEnabled(bool enabled) {
    m_enabled = enabled;
    qDebug() << "Alert System" << (enabled ? "enabled" : "disabled");
}
