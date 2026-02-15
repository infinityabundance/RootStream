/*
 * Recording Manager Wrapper for Qt - Implementation
 */

#include "recording_manager_wrapper.h"
#include <QDir>
#include <QDebug>

RecordingManagerWrapper::RecordingManagerWrapper(QObject *parent)
    : QObject(parent)
    , m_manager(nullptr)
    , m_updateTimer(new QTimer(this))
    , m_initialized(false)
    , m_replayBufferEnabled(false)
    , m_status("Not initialized")
    , m_duration(0)
    , m_fileSize(0)
{
    m_manager = new RecordingManager();
    
    // Setup update timer
    connect(m_updateTimer, &QTimer::timeout, this, &RecordingManagerWrapper::updateStatus);
    m_updateTimer->setInterval(500); // Update every 500ms
}

RecordingManagerWrapper::~RecordingManagerWrapper()
{
    if (m_manager) {
        if (m_manager->is_recording_active()) {
            m_manager->stop_recording();
        }
        m_manager->cleanup();
        delete m_manager;
    }
}

bool RecordingManagerWrapper::initialize(const QString &outputDirectory)
{
    if (m_initialized) {
        return true;
    }
    
    QByteArray dirBytes = outputDirectory.toUtf8();
    int ret = m_manager->init(dirBytes.constData());
    
    if (ret == 0) {
        m_initialized = true;
        m_status = "Initialized";
        emit statusChanged(m_status);
        qDebug() << "RecordingManager initialized with output directory:" << outputDirectory;
        return true;
    }
    
    m_status = "Initialization failed";
    emit statusChanged(m_status);
    emit recordingError("Failed to initialize recording manager");
    return false;
}

bool RecordingManagerWrapper::startRecording(int preset, const QString &gameName)
{
    if (!m_initialized) {
        emit recordingError("Recording manager not initialized");
        return false;
    }
    
    if (m_manager->is_recording_active()) {
        emit recordingError("Recording already active");
        return false;
    }
    
    QByteArray nameBytes;
    const char *name = nullptr;
    if (!gameName.isEmpty()) {
        nameBytes = gameName.toUtf8();
        name = nameBytes.constData();
    }
    
    int ret = m_manager->start_recording(static_cast<RecordingPreset>(preset), name);
    
    if (ret == 0) {
        m_updateTimer->start();
        m_status = "Recording";
        emitStateChanges();
        
        const recording_info_t *info = m_manager->get_active_recording();
        if (info) {
            emit recordingStarted(QString::fromUtf8(info->filename));
        }
        
        qDebug() << "Recording started with preset" << preset;
        return true;
    }
    
    emit recordingError("Failed to start recording");
    return false;
}

bool RecordingManagerWrapper::stopRecording()
{
    if (!m_manager->is_recording_active()) {
        return false;
    }
    
    int ret = m_manager->stop_recording();
    
    if (ret == 0) {
        m_updateTimer->stop();
        m_status = "Stopped";
        m_duration = 0;
        m_fileSize = 0;
        emitStateChanges();
        emit recordingStopped();
        
        qDebug() << "Recording stopped";
        return true;
    }
    
    emit recordingError("Failed to stop recording");
    return false;
}

bool RecordingManagerWrapper::pauseRecording()
{
    if (!m_manager->is_recording_active() || m_manager->is_recording_paused()) {
        return false;
    }
    
    int ret = m_manager->pause_recording();
    
    if (ret == 0) {
        m_status = "Paused";
        emitStateChanges();
        qDebug() << "Recording paused";
        return true;
    }
    
    emit recordingError("Failed to pause recording");
    return false;
}

bool RecordingManagerWrapper::resumeRecording()
{
    if (!m_manager->is_recording_active() || !m_manager->is_recording_paused()) {
        return false;
    }
    
    int ret = m_manager->resume_recording();
    
    if (ret == 0) {
        m_status = "Recording";
        emitStateChanges();
        qDebug() << "Recording resumed";
        return true;
    }
    
    emit recordingError("Failed to resume recording");
    return false;
}

bool RecordingManagerWrapper::enableReplayBuffer(quint32 durationSeconds, quint32 maxMemoryMB)
{
    if (!m_initialized) {
        emit recordingError("Recording manager not initialized");
        return false;
    }
    
    int ret = m_manager->enable_replay_buffer(durationSeconds, maxMemoryMB);
    
    if (ret == 0) {
        m_replayBufferEnabled = true;
        emit replayBufferStateChanged(true);
        qDebug() << "Replay buffer enabled:" << durationSeconds << "seconds," << maxMemoryMB << "MB";
        return true;
    }
    
    emit recordingError("Failed to enable replay buffer");
    return false;
}

bool RecordingManagerWrapper::disableReplayBuffer()
{
    int ret = m_manager->disable_replay_buffer();
    
    if (ret == 0) {
        m_replayBufferEnabled = false;
        emit replayBufferStateChanged(false);
        qDebug() << "Replay buffer disabled";
        return true;
    }
    
    return false;
}

bool RecordingManagerWrapper::saveReplayBuffer(const QString &filename, quint32 durationSec)
{
    if (!m_replayBufferEnabled) {
        emit recordingError("Replay buffer not enabled");
        return false;
    }
    
    QByteArray filenameBytes = filename.toUtf8();
    int ret = m_manager->save_replay_buffer(filenameBytes.constData(), durationSec);
    
    if (ret == 0) {
        emit replayBufferSaved(filename);
        qDebug() << "Replay buffer saved to:" << filename;
        return true;
    }
    
    emit recordingError("Failed to save replay buffer");
    return false;
}

bool RecordingManagerWrapper::saveReplayBufferWithCodec(const QString &filename, quint32 durationSec, int codec)
{
    if (!m_replayBufferEnabled) {
        emit recordingError("Replay buffer not enabled");
        return false;
    }
    
    QByteArray filenameBytes = filename.toUtf8();
    int ret = m_manager->save_replay_buffer(filenameBytes.constData(), durationSec, 
                                           static_cast<VideoCodec>(codec));
    
    if (ret == 0) {
        emit replayBufferSaved(filename);
        qDebug() << "Replay buffer saved to:" << filename << "with codec" << codec;
        return true;
    }
    
    emit recordingError("Failed to save replay buffer");
    return false;
}

bool RecordingManagerWrapper::addChapterMarker(const QString &title, const QString &description)
{
    if (!m_manager->is_recording_active()) {
        emit recordingError("Not recording");
        return false;
    }
    
    QByteArray titleBytes = title.toUtf8();
    QByteArray descBytes = description.toUtf8();
    
    const char *desc = description.isEmpty() ? nullptr : descBytes.constData();
    int ret = m_manager->add_chapter_marker(titleBytes.constData(), desc);
    
    if (ret == 0) {
        emit chapterMarkerAdded(title);
        qDebug() << "Chapter marker added:" << title;
        return true;
    }
    
    emit recordingError("Failed to add chapter marker");
    return false;
}

bool RecordingManagerWrapper::setGameName(const QString &name)
{
    QByteArray nameBytes = name.toUtf8();
    int ret = m_manager->set_game_name(nameBytes.constData());
    
    if (ret == 0) {
        qDebug() << "Game name set to:" << name;
        return true;
    }
    
    return false;
}

bool RecordingManagerWrapper::setOutputDirectory(const QString &directory)
{
    QByteArray dirBytes = directory.toUtf8();
    int ret = m_manager->set_output_directory(dirBytes.constData());
    
    if (ret == 0) {
        qDebug() << "Output directory set to:" << directory;
        return true;
    }
    
    return false;
}

bool RecordingManagerWrapper::setMaxStorage(quint64 maxMB)
{
    int ret = m_manager->set_max_storage(maxMB);
    
    if (ret == 0) {
        qDebug() << "Max storage set to:" << maxMB << "MB";
        return true;
    }
    
    return false;
}

bool RecordingManagerWrapper::setAutoCleanup(bool enabled, quint32 thresholdPercent)
{
    int ret = m_manager->set_auto_cleanup(enabled, thresholdPercent);
    
    if (ret == 0) {
        qDebug() << "Auto cleanup:" << (enabled ? "enabled" : "disabled") << "threshold:" << thresholdPercent << "%";
        return true;
    }
    
    return false;
}

bool RecordingManagerWrapper::isRecording() const
{
    return m_manager && m_manager->is_recording_active();
}

bool RecordingManagerWrapper::isPaused() const
{
    return m_manager && m_manager->is_recording_paused();
}

bool RecordingManagerWrapper::replayBufferEnabled() const
{
    return m_replayBufferEnabled;
}

QString RecordingManagerWrapper::recordingStatus() const
{
    return m_status;
}

qint64 RecordingManagerWrapper::recordingDuration() const
{
    return m_duration;
}

qint64 RecordingManagerWrapper::fileSize() const
{
    return m_fileSize;
}

quint64 RecordingManagerWrapper::getAvailableDiskSpace() const
{
    if (m_manager) {
        return m_manager->get_available_disk_space();
    }
    return 0;
}

quint32 RecordingManagerWrapper::getEncodingQueueDepth() const
{
    if (m_manager) {
        return m_manager->get_encoding_queue_depth();
    }
    return 0;
}

quint32 RecordingManagerWrapper::getFrameDropCount() const
{
    if (m_manager) {
        return m_manager->get_frame_drop_count();
    }
    return 0;
}

void RecordingManagerWrapper::updateStatus()
{
    if (!m_manager || !m_manager->is_recording_active()) {
        return;
    }
    
    const recording_info_t *info = m_manager->get_active_recording();
    if (info) {
        qint64 newDuration = info->duration_us / 1000000; // Convert to seconds
        qint64 newFileSize = m_manager->get_current_file_size();
        
        if (newDuration != m_duration) {
            m_duration = newDuration;
            emit durationChanged(m_duration);
        }
        
        if (newFileSize != m_fileSize) {
            m_fileSize = newFileSize;
            emit fileSizeChanged(m_fileSize);
        }
    }
}

void RecordingManagerWrapper::emitStateChanges()
{
    emit recordingStateChanged(isRecording());
    emit pauseStateChanged(isPaused());
    emit statusChanged(m_status);
}
