# Integration Example: Adding Metrics to VideoRenderer

This document shows how to integrate the Performance Metrics System with the existing `VideoRenderer` class.

## Step 1: Add MetricsManager to VideoRenderer

**File: `videorenderer.h`**

```cpp
#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#include <QObject>
#include <QOpenGLWidget>
#include <QPainter>

// Add metrics include
#ifdef ENABLE_METRICS
#include "metrics/metrics_manager.h"
#endif

class VideoRenderer : public QOpenGLWidget {
    Q_OBJECT
    
public:
    explicit VideoRenderer(QWidget* parent = nullptr);
    ~VideoRenderer();
    
    void initialize();
    void renderFrame(const uint8_t* data, int width, int height);
    
    // Metrics control
    void toggleHUD();
    void setMetricsEnabled(bool enabled);
    
protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void keyPressEvent(QKeyEvent* event) override;
    
private:
    // Existing members...
    GLuint m_textureId;
    int m_frameWidth;
    int m_frameHeight;
    
#ifdef ENABLE_METRICS
    MetricsManager* m_metrics;
#endif
};

#endif // VIDEORENDERER_H
```

## Step 2: Initialize Metrics in Constructor

**File: `videorenderer.cpp`**

```cpp
#include "videorenderer.h"
#include <QKeyEvent>
#include <QDebug>

VideoRenderer::VideoRenderer(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_textureId(0)
    , m_frameWidth(0)
    , m_frameHeight(0)
#ifdef ENABLE_METRICS
    , m_metrics(nullptr)
#endif
{
}

VideoRenderer::~VideoRenderer() {
#ifdef ENABLE_METRICS
    if (m_metrics) {
        m_metrics->cleanup();
    }
#endif
}

void VideoRenderer::initialize() {
#ifdef ENABLE_METRICS
    // Initialize metrics system
    m_metrics = new MetricsManager(this);
    if (!m_metrics->init(width(), height())) {
        qWarning() << "Failed to initialize metrics system";
        delete m_metrics;
        m_metrics = nullptr;
    } else {
        // Enable HUD by default
        m_metrics->setHUDVisible(true);
        
        // Enable logging (optional)
        QString logFile = QString("/tmp/rootstream_metrics_%1.csv")
                          .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
        m_metrics->setLoggingEnabled(true, logFile);
        
        // Enable alerts
        m_metrics->setAlertsEnabled(true);
        
        // Connect to alerts
        connect(m_metrics, &MetricsManager::fpsDropDetected,
                this, [](uint32_t fps) {
            qWarning() << "FPS drop detected:" << fps;
        });
        
        connect(m_metrics, &MetricsManager::thermalWarning,
                this, [](const QString& component, uint8_t temp) {
            qWarning() << "Thermal warning:" << component << "at" << temp << "°C";
        });
        
        qDebug() << "Metrics system initialized. Logging to:" << logFile;
    }
#endif
}

void VideoRenderer::paintGL() {
    // Clear and render your video frame as usual
    glClear(GL_COLOR_BUFFER_BIT);
    
    // ... Your existing rendering code ...
    // glBindTexture(GL_TEXTURE_2D, m_textureId);
    // ... draw textured quad ...
    
#ifdef ENABLE_METRICS
    // Record frame for metrics
    if (m_metrics) {
        m_metrics->recordFrame();
        
        // Render HUD overlay
        QPainter painter(this);
        painter.beginNativePainting();
        m_metrics->renderHUD(&painter);
        painter.endNativePainting();
    }
#endif
}

void VideoRenderer::renderFrame(const uint8_t* data, int width, int height) {
    // Update texture with new frame data
    m_frameWidth = width;
    m_frameHeight = height;
    
    // ... Your existing frame upload code ...
    
    // Trigger repaint
    update();
}
```

## Step 3: Add Keyboard Shortcuts

```cpp
void VideoRenderer::keyPressEvent(QKeyEvent* event) {
#ifdef ENABLE_METRICS
    if (m_metrics) {
        if (event->key() == Qt::Key_F3) {
            // Toggle HUD visibility
            toggleHUD();
            event->accept();
            return;
        }
        else if (event->key() == Qt::Key_F4) {
            // Toggle metrics collection
            bool enabled = m_metrics->isMetricsEnabled();
            setMetricsEnabled(!enabled);
            qDebug() << "Metrics collection" << (enabled ? "disabled" : "enabled");
            event->accept();
            return;
        }
    }
#endif
    
    QOpenGLWidget::keyPressEvent(event);
}

void VideoRenderer::toggleHUD() {
#ifdef ENABLE_METRICS
    if (m_metrics) {
        bool visible = m_metrics->isHUDVisible();
        m_metrics->setHUDVisible(!visible);
        qDebug() << "HUD" << (visible ? "hidden" : "visible");
    }
#endif
}

void VideoRenderer::setMetricsEnabled(bool enabled) {
#ifdef ENABLE_METRICS
    if (m_metrics) {
        m_metrics->setMetricsEnabled(enabled);
    }
#endif
}
```

## Step 4: Record Network Metrics (in PeerManager)

**File: `peermanager.cpp`**

```cpp
#include "peermanager.h"

#ifdef ENABLE_METRICS
#include "metrics/metrics_manager.h"
extern MetricsManager* g_metrics;  // Global metrics instance
#endif

void PeerManager::onPacketReceived(const Packet& packet) {
    // ... Your existing packet handling ...
    
#ifdef ENABLE_METRICS
    if (g_metrics) {
        // Calculate RTT if this is an ACK packet
        if (packet.type == PACKET_ACK) {
            uint64_t now = getCurrentTimestampUs();
            uint64_t sent_time = getPacketSentTime(packet.sequence);
            uint32_t rtt_ms = (now - sent_time) / 1000;
            
            g_metrics->recordNetworkLatency(rtt_ms);
        }
        
        // Track packet loss
        if (hasPacketLoss()) {
            float loss_percent = calculatePacketLossPercent();
            g_metrics->recordPacketLoss(loss_percent);
        }
    }
#endif
}
```

## Step 5: Record Input Metrics (in InputManager)

**File: `inputmanager.cpp`**

```cpp
#include "inputmanager.h"

#ifdef ENABLE_METRICS
#include "metrics/metrics_manager.h"
extern MetricsManager* g_metrics;
#endif

void InputManager::processInput(const InputEvent& event) {
    uint64_t receive_time = getCurrentTimestampUs();
    
    // ... Your existing input handling ...
    
#ifdef ENABLE_METRICS
    if (g_metrics && event.client_timestamp > 0) {
        // Calculate input latency
        uint32_t latency_ms = (receive_time - event.client_timestamp) / 1000;
        g_metrics->recordInputLatency(latency_ms);
    }
#endif
}
```

## Step 6: Record A/V Sync Metrics (in AudioPlayer)

**File: `audioplayer.cpp`**

```cpp
#include "audioplayer.h"

#ifdef ENABLE_METRICS
#include "metrics/metrics_manager.h"
extern MetricsManager* g_metrics;
#endif

void AudioPlayer::checkAVSync() {
    // Get current video and audio timestamps
    uint64_t video_pts = getVideoTimestamp();
    uint64_t audio_pts = getAudioTimestamp();
    
    // Calculate sync offset
    int32_t offset_ms = (int32_t)((int64_t)audio_pts - (int64_t)video_pts) / 1000;
    
#ifdef ENABLE_METRICS
    if (g_metrics) {
        g_metrics->recordAVSyncOffset(offset_ms);
    }
#endif
    
    // ... Your existing sync correction code ...
}
```

## Complete Example: Main Application

**File: `main.cpp`**

```cpp
#include <QApplication>
#include "videorenderer.h"

#ifdef ENABLE_METRICS
#include "metrics/metrics_manager.h"
MetricsManager* g_metrics = nullptr;  // Global metrics instance
#endif

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    VideoRenderer renderer;
    renderer.setWindowTitle("RootStream Client");
    renderer.resize(1920, 1080);
    renderer.show();
    
    // Initialize video renderer (which initializes metrics)
    renderer.initialize();
    
#ifdef ENABLE_METRICS
    // Store global metrics reference for other components
    g_metrics = renderer.getMetrics();
#endif
    
    return app.exec();
}
```

## HUD Controls

Once integrated, use these keyboard shortcuts:

- **F3**: Toggle HUD visibility
- **F4**: Toggle metrics collection (pause/resume)

## Verifying Integration

1. **Compile with metrics enabled:**
   ```bash
   cmake -DENABLE_METRICS=ON ..
   make
   ```

2. **Run the application:**
   ```bash
   ./rootstream-kde-client
   ```

3. **Verify HUD appears** in the top-left corner showing:
   - FPS and frame time
   - Network latency
   - Input latency
   - GPU/CPU/RAM usage

4. **Check log file** is being created:
   ```bash
   ls -lh /tmp/rootstream_metrics_*.csv
   ```

5. **Trigger alerts** by:
   - Limiting bandwidth to increase latency
   - Stress testing to drop FPS
   - Running CPU-intensive tasks

## Performance Tips

1. **Minimize overhead**: Metrics collection uses <1% CPU
2. **Disable when not needed**: Use F4 to pause collection
3. **Limit logging frequency**: Only log every Nth frame if needed
4. **Use release builds**: Metrics are optimized for production use

## Troubleshooting

**HUD not visible?**
- Check ENABLE_METRICS is defined
- Verify init() was called successfully
- Press F3 to toggle visibility

**Metrics not updating?**
- Ensure recordFrame() is called each frame
- Check metrics are enabled (not paused)
- Verify timer is running (1-second update interval)

**Performance impact?**
- Should be <0.5ms per frame
- Check GPU driver is working correctly
- Consider reducing history size if needed
