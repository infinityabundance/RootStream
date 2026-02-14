/* MainWindow - Main Application Window with Recording Controls */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QDockWidget>

class RootStreamClient;
class RecordingManagerWrapper;
class VideoRenderer;
class QLabel;

namespace Ui {
class RecordingControlPanel;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(RootStreamClient *client, RecordingManagerWrapper *recordingManager, QWidget *parent = nullptr);
    ~MainWindow();
    
protected:
    void closeEvent(QCloseEvent *event) override;
    
private slots:
    // Menu actions
    void onConnect();
    void onDisconnect();
    void onSettings();
    void onAbout();
    void onQuit();
    
    // Recording menu actions
    void onStartRecording();
    void onStopRecording();
    void onPauseRecording();
    void onSaveReplay();
    void onAddChapter();
    void onRecordingSettings();
    
    // Status updates
    void onConnectionStateChanged();
    void onRecordingStateChanged(bool recording);
    void onReplayBufferStateChanged(bool enabled);
    void updateStatusBar();
    
private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupRecordingControls();
    void createActions();
    void updateActions();
    
    // Components
    RootStreamClient *m_client;
    RecordingManagerWrapper *m_recordingManager;
    VideoRenderer *m_videoRenderer;
    
    // UI elements
    QWidget *m_centralWidget;
    QVBoxLayout *m_mainLayout;
    QDockWidget *m_recordingDock;
    QWidget *m_recordingPanel;
    
    // Status bar widgets
    QLabel *m_connectionStatus;
    QLabel *m_recordingStatus;
    QLabel *m_fpsLabel;
    
    // Actions
    QAction *m_connectAction;
    QAction *m_disconnectAction;
    QAction *m_settingsAction;
    QAction *m_quitAction;
    
    QAction *m_startRecordingAction;
    QAction *m_stopRecordingAction;
    QAction *m_pauseRecordingAction;
    QAction *m_saveReplayAction;
    QAction *m_addChapterAction;
    QAction *m_recordingSettingsAction;
    
    // State
    bool m_isRecording;
    bool m_isConnected;
};

#endif // MAINWINDOW_H

