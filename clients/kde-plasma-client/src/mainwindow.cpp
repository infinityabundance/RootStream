/*
 * MainWindow - Main Application Window Implementation
 */

#include "mainwindow.h"
#include "rootstreamclient.h"
#include "recording_manager_wrapper.h"
#include "videorenderer.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QCloseEvent>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QSpinBox>
#include <QGridLayout>
#include <QTimer>

MainWindow::MainWindow(RootStreamClient *client, RecordingManagerWrapper *recordingManager, QWidget *parent)
    : QMainWindow(parent)
    , m_client(client)
    , m_recordingManager(recordingManager)
    , m_videoRenderer(nullptr)
    , m_isRecording(false)
    , m_isConnected(false)
{
    setWindowTitle("RootStream - KDE Plasma Client");
    resize(1280, 720);
    
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupRecordingControls();
    createActions();
    updateActions();
    
    // Connect signals
    connect(m_client, &RootStreamClient::connectedChanged, this, &MainWindow::onConnectionStateChanged);
    connect(m_recordingManager, &RecordingManagerWrapper::recordingStateChanged, this, &MainWindow::onRecordingStateChanged);
    connect(m_recordingManager, &RecordingManagerWrapper::replayBufferStateChanged, this, &MainWindow::onReplayBufferStateChanged);
    
    // Update status bar periodically
    QTimer *statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, this, &MainWindow::updateStatusBar);
    statusTimer->start(1000);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget(this);
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    
    // Create video renderer placeholder
    QLabel *videoPlaceholder = new QLabel("Video Renderer", m_centralWidget);
    videoPlaceholder->setAlignment(Qt::AlignCenter);
    videoPlaceholder->setMinimumSize(800, 600);
    videoPlaceholder->setStyleSheet("QLabel { background-color: #2c2c2c; color: #ffffff; font-size: 24px; }");
    
    m_mainLayout->addWidget(videoPlaceholder);
    setCentralWidget(m_centralWidget);
}

void MainWindow::setupMenuBar()
{
    QMenuBar *menuBar = this->menuBar();
    
    // File menu
    QMenu *fileMenu = menuBar->addMenu("&File");
    m_connectAction = fileMenu->addAction("&Connect to Peer...");
    connect(m_connectAction, &QAction::triggered, this, &MainWindow::onConnect);
    
    m_disconnectAction = fileMenu->addAction("&Disconnect");
    connect(m_disconnectAction, &QAction::triggered, this, &MainWindow::onDisconnect);
    
    fileMenu->addSeparator();
    
    m_settingsAction = fileMenu->addAction("&Settings...");
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::onSettings);
    
    fileMenu->addSeparator();
    
    m_quitAction = fileMenu->addAction("&Quit");
    m_quitAction->setShortcut(QKeySequence::Quit);
    connect(m_quitAction, &QAction::triggered, this, &MainWindow::onQuit);
    
    // Recording menu
    QMenu *recordingMenu = menuBar->addMenu("&Recording");
    m_startRecordingAction = recordingMenu->addAction("&Start Recording");
    m_startRecordingAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(m_startRecordingAction, &QAction::triggered, this, &MainWindow::onStartRecording);
    
    m_stopRecordingAction = recordingMenu->addAction("S&top Recording");
    m_stopRecordingAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    connect(m_stopRecordingAction, &QAction::triggered, this, &MainWindow::onStopRecording);
    
    m_pauseRecordingAction = recordingMenu->addAction("&Pause Recording");
    m_pauseRecordingAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    connect(m_pauseRecordingAction, &QAction::triggered, this, &MainWindow::onPauseRecording);
    
    recordingMenu->addSeparator();
    
    m_saveReplayAction = recordingMenu->addAction("Save &Replay Buffer");
    m_saveReplayAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    connect(m_saveReplayAction, &QAction::triggered, this, &MainWindow::onSaveReplay);
    
    m_addChapterAction = recordingMenu->addAction("Add &Chapter Marker");
    m_addChapterAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
    connect(m_addChapterAction, &QAction::triggered, this, &MainWindow::onAddChapter);
    
    recordingMenu->addSeparator();
    
    m_recordingSettingsAction = recordingMenu->addAction("Recording Se&ttings...");
    connect(m_recordingSettingsAction, &QAction::triggered, this, &MainWindow::onRecordingSettings);
    
    // Help menu
    QMenu *helpMenu = menuBar->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar("Main Toolbar");
    toolBar->setMovable(false);
    
    toolBar->addAction(m_connectAction);
    toolBar->addAction(m_disconnectAction);
    toolBar->addSeparator();
    toolBar->addAction(m_startRecordingAction);
    toolBar->addAction(m_stopRecordingAction);
    toolBar->addAction(m_pauseRecordingAction);
    toolBar->addSeparator();
    toolBar->addAction(m_saveReplayAction);
    toolBar->addAction(m_addChapterAction);
}

void MainWindow::setupStatusBar()
{
    QStatusBar *status = statusBar();
    
    m_connectionStatus = new QLabel("Disconnected");
    m_recordingStatus = new QLabel("Not Recording");
    m_fpsLabel = new QLabel("FPS: --");
    
    status->addWidget(m_connectionStatus);
    status->addWidget(new QLabel("|"));
    status->addWidget(m_recordingStatus);
    status->addWidget(new QLabel("|"));
    status->addWidget(m_fpsLabel);
    status->addPermanentWidget(new QLabel("RootStream v1.0"));
}

void MainWindow::setupRecordingControls()
{
    // Create recording control dock
    m_recordingDock = new QDockWidget("Recording Controls", this);
    m_recordingPanel = new QWidget(m_recordingDock);
    
    QVBoxLayout *layout = new QVBoxLayout(m_recordingPanel);
    
    // Recording preset selection
    QGroupBox *presetGroup = new QGroupBox("Recording Preset");
    QVBoxLayout *presetLayout = new QVBoxLayout(presetGroup);
    
    QComboBox *presetCombo = new QComboBox();
    presetCombo->addItem("Fast (H.264, 20Mbps)", 0);
    presetCombo->addItem("Balanced (H.264, 8Mbps)", 1);
    presetCombo->addItem("High Quality (VP9, 5Mbps)", 2);
    presetCombo->addItem("Archival (AV1, 2Mbps)", 3);
    presetCombo->setCurrentIndex(1); // Default to Balanced
    presetLayout->addWidget(presetCombo);
    
    layout->addWidget(presetGroup);
    
    // Replay buffer settings
    QGroupBox *replayGroup = new QGroupBox("Replay Buffer");
    QVBoxLayout *replayLayout = new QVBoxLayout(replayGroup);
    
    QCheckBox *replayEnabled = new QCheckBox("Enable Replay Buffer");
    replayLayout->addWidget(replayEnabled);
    
    QHBoxLayout *replayDurationLayout = new QHBoxLayout();
    replayDurationLayout->addWidget(new QLabel("Duration (seconds):"));
    QSpinBox *replayDuration = new QSpinBox();
    replayDuration->setRange(5, 300);
    replayDuration->setValue(30);
    replayDurationLayout->addWidget(replayDuration);
    replayLayout->addLayout(replayDurationLayout);
    
    QHBoxLayout *replayMemoryLayout = new QHBoxLayout();
    replayMemoryLayout->addWidget(new QLabel("Max Memory (MB):"));
    QSpinBox *replayMemory = new QSpinBox();
    replayMemory->setRange(100, 5000);
    replayMemory->setValue(500);
    replayMemoryLayout->addWidget(replayMemory);
    replayLayout->addLayout(replayMemoryLayout);
    
    connect(replayEnabled, &QCheckBox::toggled, [this, replayDuration, replayMemory](bool checked) {
        if (checked) {
            m_recordingManager->enableReplayBuffer(replayDuration->value(), replayMemory->value());
        } else {
            m_recordingManager->disableReplayBuffer();
        }
    });
    
    layout->addWidget(replayGroup);
    
    // Quick actions
    QGroupBox *actionsGroup = new QGroupBox("Quick Actions");
    QGridLayout *actionsLayout = new QGridLayout(actionsGroup);
    
    QPushButton *startBtn = new QPushButton("Start Recording");
    QPushButton *stopBtn = new QPushButton("Stop Recording");
    QPushButton *pauseBtn = new QPushButton("Pause");
    QPushButton *saveReplayBtn = new QPushButton("Save Replay");
    
    connect(startBtn, &QPushButton::clicked, [this, presetCombo]() {
        m_recordingManager->startRecording(presetCombo->currentData().toInt());
    });
    connect(stopBtn, &QPushButton::clicked, m_recordingManager, &RecordingManagerWrapper::stopRecording);
    connect(pauseBtn, &QPushButton::clicked, [this]() {
        if (m_recordingManager->isPaused()) {
            m_recordingManager->resumeRecording();
        } else {
            m_recordingManager->pauseRecording();
        }
    });
    connect(saveReplayBtn, &QPushButton::clicked, this, &MainWindow::onSaveReplay);
    
    actionsLayout->addWidget(startBtn, 0, 0);
    actionsLayout->addWidget(stopBtn, 0, 1);
    actionsLayout->addWidget(pauseBtn, 1, 0);
    actionsLayout->addWidget(saveReplayBtn, 1, 1);
    
    layout->addWidget(actionsGroup);
    layout->addStretch();
    
    m_recordingPanel->setLayout(layout);
    m_recordingDock->setWidget(m_recordingPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_recordingDock);
}

void MainWindow::createActions()
{
    // Actions are already created in setupMenuBar
}

void MainWindow::updateActions()
{
    bool connected = m_isConnected;
    bool recording = m_isRecording;
    
    m_connectAction->setEnabled(!connected);
    m_disconnectAction->setEnabled(connected);
    
    m_startRecordingAction->setEnabled(!recording);
    m_stopRecordingAction->setEnabled(recording);
    m_pauseRecordingAction->setEnabled(recording);
    m_addChapterAction->setEnabled(recording);
    m_saveReplayAction->setEnabled(m_recordingManager && m_recordingManager->replayBufferEnabled());
}

void MainWindow::onConnect()
{
    bool ok;
    QString code = QInputDialog::getText(this, "Connect to Peer",
                                        "Enter RootStream code:", QLineEdit::Normal,
                                        "", &ok);
    if (ok && !code.isEmpty()) {
        m_client->connectToPeer(code);
    }
}

void MainWindow::onDisconnect()
{
    m_client->disconnect();
}

void MainWindow::onSettings()
{
    QMessageBox::information(this, "Settings", "Settings dialog not yet implemented");
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About RootStream",
                      "RootStream KDE Plasma Client\n"
                      "Version 1.0.0\n\n"
                      "A native Qt/QML client for RootStream streaming.\n\n"
                      "Copyright (c) 2026 RootStream Project");
}

void MainWindow::onQuit()
{
    close();
}

void MainWindow::onStartRecording()
{
    m_recordingManager->startRecording(1); // Balanced preset
}

void MainWindow::onStopRecording()
{
    m_recordingManager->stopRecording();
}

void MainWindow::onPauseRecording()
{
    if (m_recordingManager->isPaused()) {
        m_recordingManager->resumeRecording();
        m_pauseRecordingAction->setText("&Pause Recording");
    } else {
        m_recordingManager->pauseRecording();
        m_pauseRecordingAction->setText("&Resume Recording");
    }
}

void MainWindow::onSaveReplay()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save Replay Buffer",
                                                   QString(), "Video Files (*.mp4 *.mkv)");
    if (!filename.isEmpty()) {
        m_recordingManager->saveReplayBuffer(filename, 0); // Save all available
    }
}

void MainWindow::onAddChapter()
{
    bool ok;
    QString title = QInputDialog::getText(this, "Add Chapter Marker",
                                         "Chapter title:", QLineEdit::Normal,
                                         "", &ok);
    if (ok && !title.isEmpty()) {
        m_recordingManager->addChapterMarker(title);
    }
}

void MainWindow::onRecordingSettings()
{
    QMessageBox::information(this, "Recording Settings", "Recording settings dialog not yet implemented");
}

void MainWindow::onConnectionStateChanged()
{
    m_isConnected = m_client->isConnected();
    updateActions();
    updateStatusBar();
}

void MainWindow::onRecordingStateChanged(bool recording)
{
    m_isRecording = recording;
    updateActions();
    updateStatusBar();
}

void MainWindow::onReplayBufferStateChanged(bool enabled)
{
    updateActions();
}

void MainWindow::updateStatusBar()
{
    // Update connection status
    if (m_isConnected) {
        m_connectionStatus->setText(QString("Connected: %1").arg(m_client->getPeerHostname()));
    } else {
        m_connectionStatus->setText("Disconnected");
    }
    
    // Update recording status
    if (m_isRecording) {
        qint64 duration = m_recordingManager->recordingDuration();
        qint64 fileSize = m_recordingManager->fileSize();
        m_recordingStatus->setText(QString("Recording: %1s (%2 MB)")
                                  .arg(duration)
                                  .arg(fileSize / (1024 * 1024)));
    } else {
        m_recordingStatus->setText("Not Recording");
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_isRecording) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Recording Active",
            "Recording is active. Stop recording and quit?",
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            m_recordingManager->stopRecording();
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}

