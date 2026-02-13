/* AudioPlayer - Audio Playback (Stub) */
#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>

class AudioPlayer : public QObject
{
    Q_OBJECT
public:
    explicit AudioPlayer(QObject *parent = nullptr) : QObject(parent) {}
};

#endif
