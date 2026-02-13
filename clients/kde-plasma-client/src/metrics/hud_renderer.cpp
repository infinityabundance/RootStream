#include "hud_renderer.h"
#include <QDebug>
#include <QPen>
#include <QBrush>

HUDRenderer::HUDRenderer(QObject* parent)
    : QObject(parent)
    , m_windowWidth(1920)
    , m_windowHeight(1080)
{
    m_hudConfig.showHUD = true;
    m_hudConfig.showFPS = true;
    m_hudConfig.showLatency = true;
    m_hudConfig.showNetwork = true;
    m_hudConfig.showResources = true;
    m_hudConfig.showAVSync = true;
    m_hudConfig.opacity = 0.85f;
}

HUDRenderer::~HUDRenderer() {
}

bool HUDRenderer::init(int window_width, int window_height) {
    m_windowWidth = window_width;
    m_windowHeight = window_height;
    
    // Setup font for HUD
    m_hudFont = QFont("Monospace", 12);
    m_hudFont.setBold(true);
    
    initializeOpenGLFunctions();
    
    qDebug() << "HUD Renderer initialized with resolution:" << window_width << "x" << window_height;
    return true;
}

void HUDRenderer::renderHUD(const metrics_snapshot_t& metrics, QPainter* painter) {
    if (!m_hudConfig.showHUD || !painter) return;
    
    painter->setFont(m_hudFont);
    painter->setOpacity(m_hudConfig.opacity);
    
    int x = 10;
    int y = 10;
    int lineHeight = 20;
    
    // Render panels
    if (m_hudConfig.showFPS) {
        renderFPSPanel(painter, metrics.fps, x, y);
        y += lineHeight;
    }
    
    if (m_hudConfig.showNetwork) {
        renderNetworkPanel(painter, metrics.network, x, y);
        y += lineHeight;
    }
    
    if (m_hudConfig.showLatency) {
        renderInputPanel(painter, metrics.input, x, y);
        y += lineHeight;
    }
    
    if (m_hudConfig.showAVSync) {
        renderAVSyncPanel(painter, metrics.av_sync, x, y);
        y += lineHeight;
    }
    
    if (m_hudConfig.showResources) {
        renderResourcesPanel(painter, metrics.gpu, metrics.cpu, metrics.memory, x, y);
    }
    
    painter->setOpacity(1.0);
}

void HUDRenderer::renderFPSPanel(QPainter* painter, const frame_rate_metrics_t& fps, int x, int& y) {
    QString text = QString("FPS: %1 | Frame: %2ms")
                       .arg(fps.fps)
                       .arg(fps.frame_time_ms, 0, 'f', 1);
    
    if (fps.frame_drops > 0) {
        text += QString(" | Drops: %1").arg(fps.frame_drops);
    }
    
    // Color code based on FPS
    QColor color;
    if (fps.fps >= 60) {
        color = QColor(0, 255, 0); // Green
    } else if (fps.fps >= 30) {
        color = QColor(255, 255, 0); // Yellow
    } else {
        color = QColor(255, 0, 0); // Red
    }
    
    painter->setPen(QPen(color));
    painter->drawText(x, y, text);
}

void HUDRenderer::renderNetworkPanel(QPainter* painter, const network_metrics_t& net, int x, int& y) {
    QString text = QString("Latency: %1ms | Loss: %2%")
                       .arg(net.rtt_ms)
                       .arg(net.packet_loss_percent, 0, 'f', 1);
    
    if (net.jitter_ms > 0) {
        text += QString(" | Jitter: %1ms").arg(net.jitter_ms);
    }
    
    // Color code based on latency
    QColor color;
    if (net.rtt_ms < 30) {
        color = QColor(0, 255, 0); // Green
    } else if (net.rtt_ms < 100) {
        color = QColor(255, 255, 0); // Yellow
    } else {
        color = QColor(255, 0, 0); // Red
    }
    
    painter->setPen(QPen(color));
    painter->drawText(x, y, text);
}

void HUDRenderer::renderInputPanel(QPainter* painter, const input_metrics_t& input, int x, int& y) {
    QString text = QString("Input: %1ms")
                       .arg(input.input_latency_ms);
    
    if (input.total_inputs > 0) {
        text += QString(" | Total: %1").arg(input.total_inputs);
    }
    
    // Color code based on input latency
    QColor color;
    if (input.input_latency_ms < 20) {
        color = QColor(0, 255, 0); // Green
    } else if (input.input_latency_ms < 50) {
        color = QColor(255, 255, 0); // Yellow
    } else {
        color = QColor(255, 0, 0); // Red
    }
    
    painter->setPen(QPen(color));
    painter->drawText(x, y, text);
}

void HUDRenderer::renderAVSyncPanel(QPainter* painter, const av_sync_metrics_t& av, int x, int& y) {
    QString text = QString("A/V Sync: %1ms")
                       .arg(av.av_sync_offset_ms);
    
    if (av.audio_underruns > 0) {
        text += QString(" | Underruns: %1").arg(av.audio_underruns);
    }
    
    // Color code based on sync offset
    QColor color;
    if (qAbs(av.av_sync_offset_ms) < 30) {
        color = QColor(0, 255, 0); // Green
    } else if (qAbs(av.av_sync_offset_ms) < 100) {
        color = QColor(255, 255, 0); // Yellow
    } else {
        color = QColor(255, 0, 0); // Red
    }
    
    painter->setPen(QPen(color));
    painter->drawText(x, y, text);
}

void HUDRenderer::renderResourcesPanel(QPainter* painter, const gpu_metrics_t& gpu, 
                                       const cpu_metrics_t& cpu, const memory_metrics_t& mem, 
                                       int x, int& y) {
    // GPU line
    QString gpuText = QString("GPU: %1%").arg(gpu.gpu_utilization);
    if (gpu.vram_total_mb > 0) {
        gpuText += QString(" | VRAM: %1/%2MB").arg(gpu.vram_used_mb).arg(gpu.vram_total_mb);
    }
    if (gpu.gpu_temp_celsius > 0) {
        gpuText += QString(" | %1°C").arg(gpu.gpu_temp_celsius);
    }
    
    QColor gpuColor = gpu.thermal_throttling ? QColor(255, 0, 0) : QColor(0, 255, 255);
    painter->setPen(QPen(gpuColor));
    painter->drawText(x, y, gpuText);
    y += 20;
    
    // CPU line
    QString cpuText = QString("CPU: %1%").arg(cpu.cpu_usage_percent);
    if (cpu.cpu_temp_celsius > 0) {
        cpuText += QString(" | %1°C").arg(cpu.cpu_temp_celsius);
    }
    cpuText += QString(" | Load: %1").arg(cpu.load_average, 0, 'f', 2);
    
    QColor cpuColor = cpu.thermal_throttling ? QColor(255, 0, 0) : QColor(0, 255, 255);
    painter->setPen(QPen(cpuColor));
    painter->drawText(x, y, cpuText);
    y += 20;
    
    // Memory line
    QString memText = QString("RAM: %1%").arg(mem.ram_usage_percent);
    if (mem.ram_total_mb > 0) {
        memText += QString(" | %1/%2MB").arg(mem.ram_used_mb).arg(mem.ram_total_mb);
    }
    if (mem.swap_used_mb > 0) {
        memText += QString(" | Swap: %1MB").arg(mem.swap_used_mb);
    }
    
    painter->setPen(QPen(QColor(0, 255, 255)));
    painter->drawText(x, y, memText);
}

void HUDRenderer::setHUDVisible(bool visible) {
    m_hudConfig.showHUD = visible;
    emit hudConfigChanged();
}

void HUDRenderer::setHUDOpacity(float opacity) {
    m_hudConfig.opacity = qBound(0.0f, opacity, 1.0f);
    emit hudConfigChanged();
}

void HUDRenderer::setShowFPS(bool show) {
    m_hudConfig.showFPS = show;
    emit hudConfigChanged();
}

void HUDRenderer::setShowLatency(bool show) {
    m_hudConfig.showLatency = show;
    emit hudConfigChanged();
}

void HUDRenderer::setShowNetwork(bool show) {
    m_hudConfig.showNetwork = show;
    emit hudConfigChanged();
}

void HUDRenderer::setShowResources(bool show) {
    m_hudConfig.showResources = show;
    emit hudConfigChanged();
}

void HUDRenderer::setShowAVSync(bool show) {
    m_hudConfig.showAVSync = show;
    emit hudConfigChanged();
}
