#include "performance_logger.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDir>

PerformanceLogger::PerformanceLogger(QObject* parent)
    : QObject(parent)
    , m_enabled(false)
    , m_csvHeaderWritten(false)
    , m_sampleCount(0)
{
}

PerformanceLogger::~PerformanceLogger() {
    finalize();
}

bool PerformanceLogger::init(const QString& filename) {
    m_filename = filename;
    
    // Ensure directory exists
    QFileInfo fileInfo(filename);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            emit logError("Failed to create directory: " + dir.absolutePath());
            return false;
        }
    }
    
    // Open CSV file
    m_csvFile.setFileName(filename);
    if (!m_csvFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit logError("Failed to open CSV file: " + filename);
        return false;
    }
    
    m_csvStream.setDevice(&m_csvFile);
    m_csvHeaderWritten = false;
    m_enabled = true;
    
    qDebug() << "Performance Logger initialized:" << filename;
    return true;
}

bool PerformanceLogger::logSnapshotCSV(const metrics_snapshot_t& metrics) {
    if (!m_enabled || !m_csvFile.isOpen()) return false;
    
    // Write header if not written yet
    if (!m_csvHeaderWritten) {
        m_csvStream << "timestamp_us,fps,frame_time_ms,frame_drops,";
        m_csvStream << "rtt_ms,jitter_ms,packet_loss_percent,";
        m_csvStream << "input_latency_ms,av_sync_offset_ms,";
        m_csvStream << "gpu_util,gpu_temp,vram_used_mb,vram_total_mb,";
        m_csvStream << "cpu_usage,cpu_temp,load_avg,";
        m_csvStream << "ram_used_mb,ram_total_mb,ram_usage_percent,swap_used_mb\n";
        m_csvHeaderWritten = true;
    }
    
    // Write data
    m_csvStream << metrics.timestamp_us << ","
                << metrics.fps.fps << ","
                << metrics.fps.frame_time_ms << ","
                << metrics.fps.frame_drops << ","
                << metrics.network.rtt_ms << ","
                << metrics.network.jitter_ms << ","
                << metrics.network.packet_loss_percent << ","
                << metrics.input.input_latency_ms << ","
                << metrics.av_sync.av_sync_offset_ms << ","
                << (int)metrics.gpu.gpu_utilization << ","
                << (int)metrics.gpu.gpu_temp_celsius << ","
                << metrics.gpu.vram_used_mb << ","
                << metrics.gpu.vram_total_mb << ","
                << (int)metrics.cpu.cpu_usage_percent << ","
                << (int)metrics.cpu.cpu_temp_celsius << ","
                << metrics.cpu.load_average << ","
                << metrics.memory.ram_used_mb << ","
                << metrics.memory.ram_total_mb << ","
                << (int)metrics.memory.ram_usage_percent << ","
                << metrics.memory.swap_used_mb << "\n";
    
    m_csvStream.flush();
    m_sampleCount++;
    
    return true;
}

bool PerformanceLogger::logSnapshotJSON(const metrics_snapshot_t& metrics) {
    if (!m_enabled) return false;
    
    QJsonObject obj;
    obj["timestamp_us"] = static_cast<qint64>(metrics.timestamp_us);
    
    // FPS metrics
    QJsonObject fps;
    fps["fps"] = static_cast<int>(metrics.fps.fps);
    fps["frame_time_ms"] = metrics.fps.frame_time_ms;
    fps["min_frame_time_ms"] = metrics.fps.min_frame_time_ms;
    fps["max_frame_time_ms"] = metrics.fps.max_frame_time_ms;
    fps["avg_frame_time_ms"] = metrics.fps.avg_frame_time_ms;
    fps["frame_drops"] = static_cast<int>(metrics.fps.frame_drops);
    fps["total_frames"] = static_cast<qint64>(metrics.fps.total_frames);
    obj["fps"] = fps;
    
    // Network metrics
    QJsonObject network;
    network["rtt_ms"] = static_cast<int>(metrics.network.rtt_ms);
    network["min_rtt_ms"] = static_cast<int>(metrics.network.min_rtt_ms);
    network["max_rtt_ms"] = static_cast<int>(metrics.network.max_rtt_ms);
    network["avg_rtt_ms"] = static_cast<int>(metrics.network.avg_rtt_ms);
    network["jitter_ms"] = static_cast<int>(metrics.network.jitter_ms);
    network["packet_loss_percent"] = metrics.network.packet_loss_percent;
    network["bandwidth_mbps"] = static_cast<int>(metrics.network.bandwidth_mbps);
    obj["network"] = network;
    
    // Input metrics
    QJsonObject input;
    input["input_latency_ms"] = static_cast<int>(metrics.input.input_latency_ms);
    input["min_input_latency_ms"] = static_cast<int>(metrics.input.min_input_latency_ms);
    input["max_input_latency_ms"] = static_cast<int>(metrics.input.max_input_latency_ms);
    input["avg_input_latency_ms"] = static_cast<int>(metrics.input.avg_input_latency_ms);
    input["total_inputs"] = static_cast<int>(metrics.input.total_inputs);
    obj["input"] = input;
    
    // A/V sync metrics
    QJsonObject av_sync;
    av_sync["av_sync_offset_ms"] = metrics.av_sync.av_sync_offset_ms;
    av_sync["audio_underruns"] = static_cast<int>(metrics.av_sync.audio_underruns);
    av_sync["sync_corrections"] = static_cast<int>(metrics.av_sync.sync_corrections);
    obj["av_sync"] = av_sync;
    
    // GPU metrics
    QJsonObject gpu;
    gpu["vram_used_mb"] = static_cast<int>(metrics.gpu.vram_used_mb);
    gpu["vram_total_mb"] = static_cast<int>(metrics.gpu.vram_total_mb);
    gpu["gpu_utilization"] = metrics.gpu.gpu_utilization;
    gpu["gpu_temp_celsius"] = metrics.gpu.gpu_temp_celsius;
    gpu["thermal_throttling"] = metrics.gpu.thermal_throttling;
    gpu["gpu_model"] = QString::fromUtf8(metrics.gpu.gpu_model);
    obj["gpu"] = gpu;
    
    // CPU metrics
    QJsonObject cpu;
    cpu["cpu_usage_percent"] = metrics.cpu.cpu_usage_percent;
    cpu["num_cores"] = metrics.cpu.num_cores;
    cpu["load_average"] = metrics.cpu.load_average;
    cpu["cpu_temp_celsius"] = metrics.cpu.cpu_temp_celsius;
    cpu["thermal_throttling"] = metrics.cpu.thermal_throttling;
    obj["cpu"] = cpu;
    
    // Memory metrics
    QJsonObject memory;
    memory["ram_used_mb"] = static_cast<int>(metrics.memory.ram_used_mb);
    memory["ram_total_mb"] = static_cast<int>(metrics.memory.ram_total_mb);
    memory["swap_used_mb"] = static_cast<int>(metrics.memory.swap_used_mb);
    memory["cache_mb"] = static_cast<int>(metrics.memory.cache_mb);
    memory["ram_usage_percent"] = metrics.memory.ram_usage_percent;
    obj["memory"] = memory;
    
    m_jsonArray.append(obj);
    m_sampleCount++;
    
    return true;
}

bool PerformanceLogger::exportJSON(const QString& output_file) {
    if (m_jsonArray.isEmpty()) {
        emit logError("No JSON data to export");
        return false;
    }
    
    QJsonDocument doc(m_jsonArray);
    QFile file(output_file);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit logError("Failed to open JSON output file: " + output_file);
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    qDebug() << "Exported" << m_jsonArray.size() << "samples to JSON:" << output_file;
    return true;
}

bool PerformanceLogger::finalize() {
    if (m_csvFile.isOpen()) {
        m_csvStream.flush();
        m_csvFile.close();
        qDebug() << "Performance Logger finalized. Total samples:" << m_sampleCount;
    }
    
    m_enabled = false;
    return true;
}

void PerformanceLogger::setEnabled(bool enabled) {
    m_enabled = enabled;
}
