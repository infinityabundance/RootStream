#include "recording_control_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QDateTime>

RecordingControlWidget::RecordingControlWidget(QWidget *parent)
    : QWidget(parent)
    , isRecording(false)
    , isPaused(false)
    , recordingStartTime(0)
    , currentDuration(0)
{
    setupUI();
    updateButtons();
    
    // Start update timer (refresh every 100ms)
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &RecordingControlWidget::updateTimer);
    updateTimer->start(100);
}

RecordingControlWidget::~RecordingControlWidget() {
    if (updateTimer) {
        updateTimer->stop();
    }
}

void RecordingControlWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Recording controls group
    QGroupBox *controlsGroup = new QGroupBox("Recording Controls");
    QVBoxLayout *controlsLayout = new QVBoxLayout(controlsGroup);
    
    // Preset selector
    QHBoxLayout *presetLayout = new QHBoxLayout();
    presetLayout->addWidget(new QLabel("Quality Preset:"));
    presetComboBox = new QComboBox();
    presetComboBox->addItem("Fast (H.264, 20 Mbps)", PRESET_FAST);
    presetComboBox->addItem("Balanced (H.264, 8-10 Mbps)", PRESET_BALANCED);
    presetComboBox->addItem("High Quality (VP9, 5-8 Mbps)", PRESET_HIGH_QUALITY);
    presetComboBox->addItem("Archival (AV1, 2-4 Mbps)", PRESET_ARCHIVAL);
    presetComboBox->setCurrentIndex(1);  // Default to Balanced
    presetLayout->addWidget(presetComboBox);
    presetLayout->addStretch();
    controlsLayout->addLayout(presetLayout);
    
    // Replay buffer checkbox
    replayBufferCheckBox = new QCheckBox("Enable Replay Buffer (30 seconds)");
    replayBufferCheckBox->setChecked(true);
    controlsLayout->addWidget(replayBufferCheckBox);
    
    // Control buttons
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    
    startStopButton = new QPushButton("Start Recording");
    startStopButton->setMinimumHeight(40);
    connect(startStopButton, &QPushButton::clicked, this, &RecordingControlWidget::onStartStopClicked);
    buttonsLayout->addWidget(startStopButton);
    
    pauseResumeButton = new QPushButton("Pause");
    pauseResumeButton->setMinimumHeight(40);
    pauseResumeButton->setEnabled(false);
    connect(pauseResumeButton, &QPushButton::clicked, this, &RecordingControlWidget::onPauseResumeClicked);
    buttonsLayout->addWidget(pauseResumeButton);
    
    controlsLayout->addLayout(buttonsLayout);
    
    // Replay buffer and chapter buttons
    QHBoxLayout *extraButtonsLayout = new QHBoxLayout();
    
    saveReplayButton = new QPushButton("Save Replay Buffer");
    connect(saveReplayButton, &QPushButton::clicked, this, &RecordingControlWidget::onSaveReplayClicked);
    extraButtonsLayout->addWidget(saveReplayButton);
    
    addChapterButton = new QPushButton("Add Chapter Marker");
    addChapterButton->setEnabled(false);
    connect(addChapterButton, &QPushButton::clicked, this, &RecordingControlWidget::onAddChapterClicked);
    extraButtonsLayout->addWidget(addChapterButton);
    
    controlsLayout->addLayout(extraButtonsLayout);
    
    mainLayout->addWidget(controlsGroup);
    
    // Status group
    QGroupBox *statusGroup = new QGroupBox("Recording Status");
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);
    
    statusLabel = new QLabel("Status: Not Recording");
    statusLayout->addWidget(statusLabel);
    
    durationLabel = new QLabel("Duration: 00:00:00");
    statusLayout->addWidget(durationLabel);
    
    fileSizeLabel = new QLabel("File Size: 0 MB");
    statusLayout->addWidget(fileSizeLabel);
    
    bitrateLabel = new QLabel("Bitrate: 0 Mbps");
    statusLayout->addWidget(bitrateLabel);
    
    // Queue status
    QHBoxLayout *queueLayout = new QHBoxLayout();
    queueDepthLabel = new QLabel("Queue: 0/512");
    queueLayout->addWidget(queueDepthLabel);
    queueProgressBar = new QProgressBar();
    queueProgressBar->setMaximum(MAX_RECORDING_QUEUE_SIZE);
    queueProgressBar->setValue(0);
    queueLayout->addWidget(queueProgressBar);
    statusLayout->addLayout(queueLayout);
    
    frameDropsLabel = new QLabel("Frame Drops: 0");
    frameDropsLabel->setStyleSheet("QLabel { color: red; }");
    statusLayout->addWidget(frameDropsLabel);
    
    mainLayout->addWidget(statusGroup);
    
    mainLayout->addStretch();
}

void RecordingControlWidget::updateButtons() {
    if (isRecording) {
        startStopButton->setText("Stop Recording");
        startStopButton->setStyleSheet("QPushButton { background-color: #d32f2f; color: white; }");
        pauseResumeButton->setEnabled(true);
        addChapterButton->setEnabled(true);
        presetComboBox->setEnabled(false);
        
        if (isPaused) {
            pauseResumeButton->setText("Resume");
            statusLabel->setText("Status: <b>PAUSED</b>");
            statusLabel->setStyleSheet("QLabel { color: orange; }");
        } else {
            pauseResumeButton->setText("Pause");
            statusLabel->setText("Status: <b>RECORDING</b>");
            statusLabel->setStyleSheet("QLabel { color: red; }");
        }
    } else {
        startStopButton->setText("Start Recording");
        startStopButton->setStyleSheet("");
        pauseResumeButton->setEnabled(false);
        pauseResumeButton->setText("Pause");
        addChapterButton->setEnabled(false);
        presetComboBox->setEnabled(true);
        statusLabel->setText("Status: Not Recording");
        statusLabel->setStyleSheet("");
    }
}

void RecordingControlWidget::onStartStopClicked() {
    if (isRecording) {
        emit stopRecordingRequested();
    } else {
        // Get filename from user
        QString defaultName = QString("recording_%1.mp4")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
        
        QString filename = QFileDialog::getSaveFileName(
            this,
            "Save Recording As",
            defaultName,
            "Video Files (*.mp4 *.mkv);;All Files (*)"
        );
        
        if (!filename.isEmpty()) {
            RecordingPreset preset = static_cast<RecordingPreset>(
                presetComboBox->currentData().toInt()
            );
            emit startRecordingRequested(preset, filename);
        }
    }
}

void RecordingControlWidget::onPauseResumeClicked() {
    if (isPaused) {
        emit resumeRecordingRequested();
    } else {
        emit pauseRecordingRequested();
    }
}

void RecordingControlWidget::onSaveReplayClicked() {
    QString filename = QFileDialog::getSaveFileName(
        this,
        "Save Replay Buffer As",
        QString("replay_%1.mp4").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "Video Files (*.mp4 *.mkv);;All Files (*)"
    );
    
    if (!filename.isEmpty()) {
        emit replayBufferSaveRequested();
    }
}

void RecordingControlWidget::onAddChapterClicked() {
    bool ok;
    QString title = QInputDialog::getText(
        this,
        "Add Chapter Marker",
        "Chapter title:",
        QLineEdit::Normal,
        "",
        &ok
    );
    
    if (ok && !title.isEmpty()) {
        emit chapterMarkerRequested(title);
    }
}

void RecordingControlWidget::updateTimer() {
    if (isRecording && !isPaused) {
        // Update duration display
        uint64_t elapsed_us = QDateTime::currentMSecsSinceEpoch() * 1000 - recordingStartTime;
        currentDuration = elapsed_us;
        durationLabel->setText(QString("Duration: %1").arg(formatDuration(elapsed_us)));
    }
}

void RecordingControlWidget::setRecordingActive(bool active) {
    isRecording = active;
    if (active) {
        recordingStartTime = QDateTime::currentMSecsSinceEpoch() * 1000;
    }
    updateButtons();
}

void RecordingControlWidget::setRecordingPaused(bool paused) {
    isPaused = paused;
    updateButtons();
}

void RecordingControlWidget::updateRecordingInfo(const recording_info_t *info) {
    if (!info) return;
    
    currentDuration = info->duration_us;
    durationLabel->setText(QString("Duration: %1").arg(formatDuration(info->duration_us)));
    fileSizeLabel->setText(QString("File Size: %1").arg(formatFileSize(info->file_size_bytes)));
    
    // Calculate current bitrate
    if (info->duration_us > 0) {
        double duration_sec = info->duration_us / 1000000.0;
        double bitrate_mbps = (info->file_size_bytes * 8.0 / 1000000.0) / duration_sec;
        bitrateLabel->setText(QString("Bitrate: %1 Mbps").arg(bitrate_mbps, 0, 'f', 1));
    }
}

void RecordingControlWidget::updateStats(uint64_t file_size, uint32_t queue_depth, uint32_t frame_drops) {
    fileSizeLabel->setText(QString("File Size: %1").arg(formatFileSize(file_size)));
    queueDepthLabel->setText(QString("Queue: %1/%2").arg(queue_depth).arg(MAX_RECORDING_QUEUE_SIZE));
    queueProgressBar->setValue(queue_depth);
    
    if (frame_drops > 0) {
        frameDropsLabel->setText(QString("Frame Drops: %1").arg(frame_drops));
        frameDropsLabel->show();
    } else {
        frameDropsLabel->hide();
    }
}

QString RecordingControlWidget::formatDuration(uint64_t duration_us) {
    uint64_t total_seconds = duration_us / 1000000;
    uint64_t hours = total_seconds / 3600;
    uint64_t minutes = (total_seconds % 3600) / 60;
    uint64_t seconds = total_seconds % 60;
    
    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

QString RecordingControlWidget::formatFileSize(uint64_t bytes) {
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else if (bytes < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
}
