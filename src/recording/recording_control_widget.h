#ifndef RECORDING_CONTROL_WIDGET_H
#define RECORDING_CONTROL_WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QProgressBar>
#include <QCheckBox>

extern "C" {
#include "../recording_types.h"
}

class RecordingControlWidget : public QWidget {
    Q_OBJECT

public:
    explicit RecordingControlWidget(QWidget *parent = nullptr);
    ~RecordingControlWidget();

    // Control methods
    void setRecordingActive(bool active);
    void setRecordingPaused(bool paused);
    void updateRecordingInfo(const recording_info_t *info);
    void updateStats(uint64_t file_size, uint32_t queue_depth, uint32_t frame_drops);

signals:
    void startRecordingRequested(RecordingPreset preset, const QString &filename);
    void stopRecordingRequested();
    void pauseRecordingRequested();
    void resumeRecordingRequested();
    void replayBufferSaveRequested();
    void chapterMarkerRequested(const QString &title);

private slots:
    void onStartStopClicked();
    void onPauseResumeClicked();
    void onSaveReplayClicked();
    void onAddChapterClicked();
    void updateTimer();

private:
    void setupUI();
    void updateButtons();
    QString formatDuration(uint64_t duration_us);
    QString formatFileSize(uint64_t bytes);

    // UI elements
    QPushButton *startStopButton;
    QPushButton *pauseResumeButton;
    QPushButton *saveReplayButton;
    QPushButton *addChapterButton;
    
    QComboBox *presetComboBox;
    QCheckBox *replayBufferCheckBox;
    
    QLabel *statusLabel;
    QLabel *durationLabel;
    QLabel *fileSizeLabel;
    QLabel *bitrateLabel;
    QLabel *queueDepthLabel;
    QLabel *frameDropsLabel;
    
    QProgressBar *queueProgressBar;
    
    QTimer *updateTimer;
    
    // State
    bool isRecording;
    bool isPaused;
    uint64_t recordingStartTime;
    uint64_t currentDuration;
};

#endif /* RECORDING_CONTROL_WIDGET_H */
