/*
 * videorenderer.cpp — QQuickFramebufferObject-based NV12 video renderer
 *
 * See videorenderer.h for design rationale and threading model.
 *
 * OPENGL NV12 RENDERING PIPELINE
 * --------------------------------
 * NV12 is a bi-planar YUV format:
 *   Plane 0: Y luma,   width × height bytes, 1 byte per pixel
 *   Plane 1: UV chroma, width/2 × height/2 pairs, 2 bytes per pair
 *
 * We upload each plane to a separate GL texture:
 *   tex_y_:  GL_RED,  width × height
 *   tex_uv_: GL_RG,   width/2 × height/2
 *
 * Then a quad shader converts YUV → RGB using the BT.709 matrix.
 * BT.709 is the standard for HD video (720p and above, which is what
 * RootStream typically streams).  BT.601 would be used for SD content.
 *
 * SHADER SOURCE
 * -------------
 * The shader is embedded as a string constant so there are no file I/O
 * dependencies at runtime.  This is the standard pattern for Qt Quick
 * OpenGL items that must work without a resource system.
 *
 * FUTURE UPGRADE PATH
 * -------------------
 * Replace uploadNV12() with:
 *   1. EGL_EXT_image_dma_buf_import (Linux zero-copy from VA-API)
 *   2. VK_EXT_external_memory_dma_buf (Vulkan path)
 * This will eliminate the CPU memcpy that currently happens in submitFrame().
 */

#include "videorenderer.h"
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QDateTime>
#include <QDebug>
#include <cstring>

/* ── GLSL shader sources ──────────────────────────────────────────── */

/*
 * Vertex shader: full-screen quad via two triangles.
 * gl_Position covers the NDC quad [-1,-1] → [1,1].
 * texCoord is passed to the fragment shader for texture sampling.
 */
static const char *kVertexShaderSrc = R"(
#version 300 es
precision mediump float;

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texcoord;

out vec2 v_texcoord;

void main() {
    gl_Position = vec4(a_position, 0.0, 1.0);
    v_texcoord  = a_texcoord;
}
)";

/*
 * Fragment shader: NV12 → sRGB conversion.
 *
 * BT.709 matrix (HD video standard):
 *   R = 1.164 * (Y - 16/255) + 1.793 * (V - 0.5)
 *   G = 1.164 * (Y - 16/255) - 0.213 * (U - 0.5) - 0.533 * (V - 0.5)
 *   B = 1.164 * (Y - 16/255) + 2.112 * (U - 0.5)
 *
 * Where:
 *   Y  = tex_y.r     (red channel of single-channel luma texture)
 *   U  = tex_uv.r    (red channel of RG chroma texture)
 *   V  = tex_uv.g    (green channel of RG chroma texture)
 */
static const char *kFragmentShaderSrc = R"(
#version 300 es
precision mediump float;

uniform sampler2D tex_y;   /* Luma plane (GL_RED, width × height)         */
uniform sampler2D tex_uv;  /* Chroma plane (GL_RG, width/2 × height/2)    */

in  vec2 v_texcoord;
out vec4 frag_color;

void main() {
    float y  = texture(tex_y,  v_texcoord).r;
    vec2  uv = texture(tex_uv, v_texcoord).rg;

    /* BT.709 YUV-to-RGB conversion */
    float r = clamp(1.164 * (y - 0.0627) + 1.793 * (uv.g - 0.5), 0.0, 1.0);
    float g = clamp(1.164 * (y - 0.0627) - 0.213 * (uv.r - 0.5)
                                          - 0.533 * (uv.g - 0.5), 0.0, 1.0);
    float b = clamp(1.164 * (y - 0.0627) + 2.112 * (uv.r - 0.5), 0.0, 1.0);

    frag_color = vec4(r, g, b, 1.0);
}
)";

/* ── Full-screen quad vertices ────────────────────────────────────── */

/* Two triangles forming a full-screen quad.
 * Position (xy) in NDC [-1,1] followed by texture coordinate (uv) in [0,1].
 * Texture V-axis is flipped (1-y) to account for OpenGL's bottom-left origin
 * vs the top-left origin of decoded video frames. */
static const float kQuadVertices[] = {
    /* position     texcoord */
    -1.0f, -1.0f,   0.0f, 1.0f,
     1.0f, -1.0f,   1.0f, 1.0f,
    -1.0f,  1.0f,   0.0f, 0.0f,
     1.0f, -1.0f,   1.0f, 1.0f,
     1.0f,  1.0f,   1.0f, 0.0f,
    -1.0f,  1.0f,   0.0f, 0.0f,
};

/* ── VideoRenderer ────────────────────────────────────────────────── */

VideoRenderer::VideoRenderer(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    /* setMirrorVertically(false) is the default — our shader handles
     * the V-axis flip in the texture coordinates above. */
    setFlag(ItemHasContents, true);
}

VideoRenderer::~VideoRenderer() = default;

QQuickFramebufferObject::Renderer *VideoRenderer::createRenderer() const {
    /* Called on the Qt render thread when the item is first shown.
     * Returns a new VideoRendererGL which owns all GL state. */
    return new VideoRendererGL();
}

void VideoRenderer::submitFrame(const QByteArray &nv12_data,
                                int               width,
                                int               height)
{
    /* Called from the GUI thread (via QueuedConnection from
     * StreamBackendConnector).  Stores the frame data and requests a
     * redraw.  The Renderer reads pending_frame_ in synchronize(),
     * which Qt calls on the render thread while the GUI thread is blocked. */
    pending_frame_  = nv12_data;
    pending_width_  = width;
    pending_height_ = height;

    if (!stream_active_) {
        stream_active_ = true;
        emit streamActiveChanged();
    }

    if (frame_width_ != width || frame_height_ != height) {
        frame_width_  = width;
        frame_height_ = height;
        emit frameSizeChanged();
    }

    /* FPS tracking: count frames in 1-second windows */
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (fps_timer_start_ == 0) fps_timer_start_ = now;
    fps_frame_count_++;
    if (now - fps_timer_start_ >= 1000) {
        fps_ = static_cast<double>(fps_frame_count_) * 1000.0
               / static_cast<double>(now - fps_timer_start_);
        fps_frame_count_ = 0;
        fps_timer_start_ = now;
        emit fpsChanged();
    }

    /* Request a re-render.  This posts a QEvent to the GUI thread that
     * causes Qt to call Renderer::render() on the render thread. */
    update();
}

/* ── VideoRendererGL ──────────────────────────────────────────────── */

VideoRenderer::VideoRendererGL::VideoRendererGL() = default;

VideoRenderer::VideoRendererGL::~VideoRendererGL() {
    /* Clean up GL resources.  This destructor runs on the render thread
     * so GL calls are valid here. */
    if (tex_y_)  glDeleteTextures(1, &tex_y_);
    if (tex_uv_) glDeleteTextures(1, &tex_uv_);
    /* VAO/VBO cleanup would go here if using a desktop GL profile */
    delete shader_;
}

QOpenGLFramebufferObject *
VideoRenderer::VideoRendererGL::createFramebufferObject(const QSize &size) {
    /* MSAA is disabled to avoid colour-space artefacts from multisample
     * resolve on YUV textures. */
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    return new QOpenGLFramebufferObject(size, format);
}

void VideoRenderer::VideoRendererGL::synchronize(
    QQuickFramebufferObject *item)
{
    /* Called on the render thread while GUI thread is blocked.
     * Safely copies the pending frame from VideoRenderer to this Renderer. */
    VideoRenderer *vr = static_cast<VideoRenderer *>(item);
    if (!vr->pending_frame_.isEmpty()) {
        pending_data_ = vr->pending_frame_;
        pending_w_    = vr->pending_width_;
        pending_h_    = vr->pending_height_;
    }
}

void VideoRenderer::VideoRendererGL::initGL() {
    if (gl_initialised_) return;

    initializeOpenGLFunctions();

    /* Compile NV12→RGB shader */
    shader_ = new QOpenGLShaderProgram();
    if (!shader_->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                          kVertexShaderSrc) ||
        !shader_->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                          kFragmentShaderSrc) ||
        !shader_->link()) {
        qWarning() << "VideoRenderer: shader compile failed:"
                   << shader_->log();
        delete shader_;
        shader_ = nullptr;
        return;
    }

    shader_->bind();
    shader_->setUniformValue("tex_y",  0);  /* texture unit 0 = luma   */
    shader_->setUniformValue("tex_uv", 1);  /* texture unit 1 = chroma */
    shader_->release();

    /* Create the two NV12 textures — dimensions are set in uploadNV12() */
    glGenTextures(1, &tex_y_);
    glGenTextures(1, &tex_uv_);

    /* Set texture parameters for both textures (clamp + linear filter) */
    for (GLuint tex : {tex_y_, tex_uv_}) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    gl_initialised_ = true;
}

void VideoRenderer::VideoRendererGL::uploadNV12(const QByteArray &data,
                                                int width, int height)
{
    if (data.isEmpty() || width <= 0 || height <= 0) return;
    if (data.size() < width * height * 3 / 2) {
        qWarning() << "VideoRenderer: NV12 buffer too small";
        return;
    }

    const uint8_t *y_ptr  = reinterpret_cast<const uint8_t *>(data.constData());
    const uint8_t *uv_ptr = y_ptr + width * height;

    /* Upload Y plane: width × height, one byte per pixel → GL_RED */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_y_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                 width, height, 0,
                 GL_RED, GL_UNSIGNED_BYTE, y_ptr);

    /* Upload UV plane: width/2 × height/2, two bytes per pair → GL_RG */
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex_uv_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG,
                 width / 2, height / 2, 0,
                 GL_RG, GL_UNSIGNED_BYTE, uv_ptr);
}

void VideoRenderer::VideoRendererGL::drawQuad() {
    if (!shader_) return;

    shader_->bind();

    /* Draw the full-screen quad using immediate-mode compatible path.
     * We use glVertexAttribPointer directly to avoid VAO/VBO setup
     * complexity on OpenGL ES 3.0 (which Qt Quick targets on Wayland). */
    GLuint pos_loc = (GLuint)shader_->attributeLocation("a_position");
    GLuint uv_loc  = (GLuint)shader_->attributeLocation("a_texcoord");

    glEnableVertexAttribArray(pos_loc);
    glEnableVertexAttribArray(uv_loc);

    glVertexAttribPointer(pos_loc, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(float), kQuadVertices);
    glVertexAttribPointer(uv_loc,  2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(float), kQuadVertices + 2);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(pos_loc);
    glDisableVertexAttribArray(uv_loc);

    shader_->release();
}

void VideoRenderer::VideoRendererGL::render() {
    initGL();
    if (!gl_initialised_) return;

    /* Upload the most recent frame data if available */
    if (!pending_data_.isEmpty()) {
        uploadNV12(pending_data_, pending_w_, pending_h_);
        pending_data_.clear();
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    /* Only draw if we have valid textures (i.e., at least one frame has
     * been uploaded) */
    if (tex_y_ && tex_uv_) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_y_);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_uv_);
        drawQuad();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

