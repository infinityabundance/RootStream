#ifndef HUD_RENDERER_H
#define HUD_RENDERER_H

#include <QObject>
#include <QOpenGLFunctions>
#include <QFont>
#include <QPainter>
#include <QImage>
#include "metrics_types.h"

class HUDRenderer : public QObject, protected QOpenGLFunctions {
    Q_OBJECT
    
public:
    explicit HUDRenderer(QObject* parent = nullptr);
    ~HUDRenderer();
    
    // Initialize HUD renderer
    bool init(int window_width, int window_height);
    
    // Render HUD overlay
    void renderHUD(const metrics_snapshot_t& metrics, QPainter* painter);
    
    // HUD configuration
    void setHUDVisible(bool visible);
    void setHUDOpacity(float opacity);
    void setShowFPS(bool show);
    void setShowLatency(bool show);
    void setShowNetwork(bool show);
    void setShowResources(bool show);
    void setShowAVSync(bool show);
    
    bool isHUDVisible() const { return m_hudConfig.showHUD; }
    
signals:
    void hudConfigChanged();
    
private:
    void renderFPSPanel(QPainter* painter, const frame_rate_metrics_t& fps, int x, int& y);
    void renderNetworkPanel(QPainter* painter, const network_metrics_t& net, int x, int& y);
    void renderInputPanel(QPainter* painter, const input_metrics_t& input, int x, int& y);
    void renderResourcesPanel(QPainter* painter, const gpu_metrics_t& gpu, 
                              const cpu_metrics_t& cpu, const memory_metrics_t& mem, 
                              int x, int& y);
    void renderAVSyncPanel(QPainter* painter, const av_sync_metrics_t& av, int x, int& y);
    
    struct HUDConfig {
        bool showHUD;
        bool showFPS;
        bool showLatency;
        bool showNetwork;
        bool showResources;
        bool showAVSync;
        float opacity;
    } m_hudConfig;
    
    QFont m_hudFont;
    int m_windowWidth;
    int m_windowHeight;
};

#endif // HUD_RENDERER_H
