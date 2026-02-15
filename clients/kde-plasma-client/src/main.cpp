/*
 * RootStream KDE Plasma Client - Main Entry Point
 * 
 * Copyright (c) 2026 RootStream Project
 * Licensed under MIT License
 */

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineParser>
#include <QIcon>
#include <iostream>

#include "rootstreamclient.h"
#include "peermanager.h"
#include "settingsmanager.h"
#include "logmanager.h"
#include "mainwindow.h"
#include "recording_manager_wrapper.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("RootStream KDE Client");
    app.setOrganizationName("RootStream");
    app.setApplicationVersion("1.0.0");
    
    // Set application icon
    app.setWindowIcon(QIcon(":/icons/rootstream.svg"));
    
    // Command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("RootStream KDE Plasma Native Client");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // AI logging option
    QCommandLineOption aiLoggingOption(
        QStringList() << "ai-logging" << "ai-log",
        "Enable AI logging mode for debugging"
    );
    parser.addOption(aiLoggingOption);
    
    // Auto-connect option
    QCommandLineOption autoConnectOption(
        "connect",
        "Auto-connect to peer on startup",
        "code"
    );
    parser.addOption(autoConnectOption);
    
    // Recording options
    QCommandLineOption outputDirOption(
        "output-dir",
        "Output directory for recordings",
        "path",
        QDir::homePath() + "/Videos/RootStream"
    );
    parser.addOption(outputDirOption);
    
    QCommandLineOption replayBufferOption(
        "replay-buffer-seconds",
        "Enable replay buffer with specified duration",
        "seconds",
        "30"
    );
    parser.addOption(replayBufferOption);
    
    parser.process(app);
    
    // Initialize components
    SettingsManager settingsManager;
    LogManager logManager;
    RootStreamClient client;
    PeerManager peerManager(&client);
    
    // Initialize recording manager
    RecordingManagerWrapper recordingManager;
    QString outputDir = parser.value(outputDirOption);
    if (!recordingManager.initialize(outputDir)) {
        std::cerr << "Warning: Failed to initialize recording manager" << std::endl;
    }
    
    // Enable replay buffer if requested
    if (parser.isSet(replayBufferOption)) {
        int duration = parser.value(replayBufferOption).toInt();
        if (duration > 0) {
            recordingManager.enableReplayBuffer(duration, 500); // 500MB default
            std::cout << "Replay buffer enabled: " << duration << " seconds" << std::endl;
        }
    }
    
    // Enable AI logging if requested
    if (parser.isSet(aiLoggingOption)) {
        logManager.setEnabled(true);
        client.setAILoggingEnabled(true);
        std::cout << "AI logging mode enabled" << std::endl;
    }
    
    // Load settings
    settingsManager.load();
    
    // Apply settings to client
    if (settingsManager.hasCodec()) {
        client.setVideoCodec(settingsManager.getCodec());
    }
    if (settingsManager.hasBitrate()) {
        client.setBitrate(settingsManager.getBitrate());
    }
    
    // Create main window
    MainWindow mainWindow(&client, &recordingManager);
    mainWindow.show();
    
    // Auto-connect if specified
    if (parser.isSet(autoConnectOption)) {
        QString code = parser.value(autoConnectOption);
        QMetaObject::invokeMethod(&client, "connectToPeer", Qt::QueuedConnection,
                                Q_ARG(QString, code));
    }
    
    return app.exec();
}
