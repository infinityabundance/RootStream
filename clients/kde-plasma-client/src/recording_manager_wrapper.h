/*
 * Recording Manager Wrapper for Qt
 * 
 * Wraps the C-based RecordingManager for Qt/QML integration
 */

#ifndef RECORDING_MANAGER_WRAPPER_H
#define RECORDING_MANAGER_WRAPPER_H

#include <QObject>
#include <QString>
#include <QTimer>

extern "C" {
#include "../../src/recording/recording_manager.h"
#include "../../src/recording/recording_types.h"
}

class RecordingManagerWrapper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY recordingStateChanged)
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY pauseStateChanged)
    Q_PROPERTY(bool replayBufferEnabled READ replayBufferEnabled NOTIFY replayBufferStateChanged)
    Q_PROPERTY(QString recordingStatus READ recordingStatus NOTIFY statusChanged)
    Q_PROPERTY(qint64 recordingDuration READ recordingDuration NOTIFY durationChanged)
    Q_PROPERTY(qint64 fileSize READ fileSize NOTIFY fileSizeChanged)
    
public:
    explicit RecordingManagerWrapper(QObject *parent = nullptr);
    ~RecordingManagerWrapper();
    
    // Initialization
    Q_INVOKABLE bool initialize(const QString &outputDirectory);
    
    // Recording control
    Q_INVOKABLE bool startRecording(int preset, const QString &gameName = QString());
    Q_INVOKABLE bool stopRecording();
    Q_INVOKABLE bool pauseRecording();
    Q_INVOKABLE bool resumeRecording();
    
    // Replay buffer control
    Q_INVOKABLE bool enableReplayBuffer(quint32 durationSeconds, quint32 maxMemoryMB);
    Q_INVOKABLE bool disableReplayBuffer();
    Q_INVOKABLE bool saveReplayBuffer(const QString &filename, quint32 durationSec);
    Q_INVOKABLE bool saveReplayBufferWithCodec(const QString &filename, quint32 durationSec, int codec);
    
    // Metadata control
    Q_INVOKABLE bool addChapterMarker(const QString &title, const QString &description = QString());
    Q_INVOKABLE bool setGameName(const QString &name);
    
    // Configuration
    Q_INVOKABLE bool setOutputDirectory(const QString &directory);
    Q_INVOKABLE bool setMaxStorage(quint64 maxMB);
    Q_INVOKABLE bool setAutoCleanup(bool enabled, quint32 thresholdPercent);
    
    // State queries
    bool isRecording() const;
    bool isPaused() const;
    bool replayBufferEnabled() const;
    QString recordingStatus() const;
    qint64 recordingDuration() const;
    qint64 fileSize() const;
    
    Q_INVOKABLE quint64 getAvailableDiskSpace() const;
    Q_INVOKABLE quint32 getEncodingQueueDepth() const;
    Q_INVOKABLE quint32 getFrameDropCount() const;
    
signals:
    void recordingStateChanged(bool recording);
    void pauseStateChanged(bool paused);
    void replayBufferStateChanged(bool enabled);
    void statusChanged(const QString &status);
    void durationChanged(qint64 duration);
    void fileSizeChanged(qint64 size);
    void recordingStarted(const QString &filename);
    void recordingStopped();
    void recordingError(const QString &error);
    void replayBufferSaved(const QString &filename);
    void chapterMarkerAdded(const QString &title);
    
private slots:
    void updateStatus();
    
private:
    RecordingManager *m_manager;
    QTimer *m_updateTimer;
    bool m_initialized;
    bool m_replayBufferEnabled;
    QString m_status;
    qint64 m_duration;
    qint64 m_fileSize;
    
    void emitStateChanges();
};

#endif // RECORDING_MANAGER_WRAPPER_H
