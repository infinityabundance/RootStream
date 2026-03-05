/*
 * videorenderer.h — QQuickFramebufferObject-based video renderer
 *
 * OVERVIEW
 * --------
 * VideoRenderer is a Qt Quick item that displays decoded video frames inside
 * the KDE Plasma client QML UI.  It bridges the RootStream backend callback
 * system (rs_video_frame_t) to the Qt rendering pipeline.
 *
 * ARCHITECTURE
 * ------------
 *
 *   [Session worker thread]                    [Qt render thread]
 *
 *   rs_client_session_run()
 *       → on_video_fn callback
 *       → StreamBackendConnector::onVideoFrame()
 *           → copies NV12 data to QByteArray
 *           → emits frameReady(QByteArray, int, int)   ─────────────────┐
 *                                                                        ↓
 *                                                          VideoRenderer::submitFrame()
 *                                                              → stores pending_frame_
 *                                                              → calls update()
 *                                                                   ↓
 *                                                    VideoRenderer::Renderer::render()
 *                                                        → upload NV12 to GL textures
 *                                                        → draw quad with NV12→RGB shader
 *
 * WHY QQuickFramebufferObject?
 *   QQuickFramebufferObject is the standard Qt Quick integration for custom
 *   OpenGL rendering.  It runs render() on the dedicated Qt render thread,
 *   handles FBO creation/resize automatically, and composites the result
 *   into the QML scene graph.
 *
 *   Alternative: QQuickItem with scene graph node.  More flexible but more
 *   code.  QFBO is the "boringly reliable first" choice per the ROADMAP.
 *
 * WHY NV12?
 *   VA-API (the primary decoder on Linux) outputs NV12.  Uploading NV12
 *   directly avoids a CPU-side colour-space conversion.  The shader does
 *   the YUV→RGB conversion on the GPU (free/fast).
 *
 * UPGRADE PATH
 *   MVP: CPU copy NV12 → GL_TEXTURE_2D (this file)
 *   Next: DMABUF import via EGL_EXT_image_dma_buf_import (zero-copy)
 *   Then: Vulkan import via VK_EXT_external_memory_dma_buf (for Vulkan path)
 *
 * THREAD-SAFETY
 * -------------
 * submitFrame() is called from the Qt render thread (via QueuedConnection
 * in StreamBackendConnector).  render() also runs on the render thread.
 * No mutex is needed because both are serialised by Qt's render thread.
 *
 * update() is posted to the GUI thread from submitFrame() — this is the
 * standard QQuickFramebufferObject pattern.
 */

#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>
#include <QByteArray>
#include <QMutex>
#include <QOpenGLShaderProgram>

/* Forward declaration — the C session API is pure C */
extern "C" {
#include "rootstream_client_session.h"
}

class VideoRenderer : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_ELEMENT

    /* ── QML-visible properties ──────────────────────────────────────── */

    /** True when a stream is active and frames are arriving */
    Q_PROPERTY(bool streamActive READ streamActive NOTIFY streamActiveChanged)

    /** Most recent frame width (pixels) */
    Q_PROPERTY(int frameWidth READ frameWidth NOTIFY frameSizeChanged)

    /** Most recent frame height (pixels) */
    Q_PROPERTY(int frameHeight READ frameHeight NOTIFY frameSizeChanged)

    /** Decoded frames per second (updated every second) */
    Q_PROPERTY(double fps READ fps NOTIFY fpsChanged)

public:
    explicit VideoRenderer(QQuickItem *parent = nullptr);
    ~VideoRenderer() override;

    /* ── QQuickFramebufferObject interface ───────────────────────────── */

    /** Called by Qt to create the Renderer object on the render thread.
     *  The Renderer owns all GL objects (textures, shaders, FBO). */
    Renderer *createRenderer() const override;

    /* ── Property getters ────────────────────────────────────────────── */
    bool   streamActive() const { return stream_active_; }
    int    frameWidth()   const { return frame_width_;   }
    int    frameHeight()  const { return frame_height_;  }
    double fps()          const { return fps_;           }

    /* ── Frame submission (called from Qt render thread) ─────────────── */

    /**
     * submitFrame — copy a decoded frame for rendering.
     *
     * Called via QueuedConnection from StreamBackendConnector::onVideoFrame().
     * Safe to call from any thread because the signal/slot mechanism copies
     * the data argument before delivering to the render thread.
     *
     * @param nv12_data  NV12 frame data (Y plane followed by UV plane)
     * @param width      Frame width in pixels
     * @param height     Frame height in pixels
     */
    Q_INVOKABLE void submitFrame(const QByteArray &nv12_data,
                                 int               width,
                                 int               height);

signals:
    void streamActiveChanged();
    void frameSizeChanged();
    void fpsChanged();

    /** Emitted when a new frame is ready for rendering.
     *  Connected to the Renderer object's updateTextures() slot on the
     *  render thread. */
    void newFrameAvailable(QByteArray nv12_data, int width, int height);

private:
    /* ── Frame data shared with Renderer ─────────────────────────────── */
    /* Protected by QQuickFramebufferObject's built-in synchronise mechanism:
     * Qt calls synchronize() on the render thread while the GUI thread is
     * blocked, ensuring consistent state. */
    QByteArray pending_frame_;
    int        pending_width_  = 0;
    int        pending_height_ = 0;

    /* ── State ────────────────────────────────────────────────────────── */
    bool   stream_active_ = false;
    int    frame_width_   = 0;
    int    frame_height_  = 0;
    double fps_           = 0.0;

    /* FPS tracking */
    int      fps_frame_count_ = 0;
    qint64   fps_timer_start_ = 0;

    /* ── Inner Renderer class ─────────────────────────────────────────── */
public:
    /**
     * VideoRendererGL — the actual OpenGL renderer.
     *
     * Owned by the Qt render thread.  Created by createRenderer().
     * Receives NV12 frame data from the parent VideoRenderer via
     * QQuickFramebufferObject::synchronize().
     *
     * NV12 GL upload strategy:
     *   - Two GL_TEXTURE_2D textures: tex_y_ (luminance) + tex_uv_ (chrominance)
     *   - tex_y_:  width × height,   GL_RED (one byte per pixel)
     *   - tex_uv_: width/2 × height/2, GL_RG  (two bytes: U and V interleaved)
     *   - A GLSL shader converts YUV to RGB using the BT.709 matrix.
     */
    class VideoRendererGL : public QQuickFramebufferObject::Renderer,
                            protected QOpenGLFunctions
    {
    public:
        VideoRendererGL();
        ~VideoRendererGL() override;

        void render() override;
        QOpenGLFramebufferObject *createFramebufferObject(
            const QSize &size) override;
        void synchronize(QQuickFramebufferObject *item) override;

    private:
        void initGL();
        void uploadNV12(const QByteArray &data, int width, int height);
        void drawQuad();

        QOpenGLShaderProgram *shader_ = nullptr;
        GLuint tex_y_  = 0;   /**< Luma texture (GL_RED)            */
        GLuint tex_uv_ = 0;   /**< Chroma texture (GL_RG)           */
        GLuint vao_    = 0;   /**< Vertex array object               */
        GLuint vbo_    = 0;   /**< Vertex buffer (quad positions+UVs)*/

        QByteArray pending_data_;
        int        pending_w_ = 0;
        int        pending_h_ = 0;
        bool       gl_initialised_ = false;
    };
};

#endif /* VIDEORENDERER_H */

