/* VideoRenderer - OpenGL Video Rendering (Stub) */
#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#include <QObject>

class VideoRenderer : public QObject
{
    Q_OBJECT
public:
    explicit VideoRenderer(QObject *parent = nullptr) : QObject(parent) {}
};

#endif
