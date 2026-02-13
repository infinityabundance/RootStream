/*
 * RootStream KDE Plasma Client - Main Entry Point
 * 
 * Copyright (c) 2026 RootStream Project
 * Licensed under MIT License
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineParser>
#include <QIcon>
#include <iostream>

#include "rootstreamclient.h"
#include "peermanager.h"
#include "settingsmanager.h"
#include "logmanager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
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
    
    parser.process(app);
    
    // Initialize components
    SettingsManager settingsManager;
    LogManager logManager;
    RootStreamClient client;
    PeerManager peerManager(&client);
    
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
    
    // Create QML engine
    QQmlApplicationEngine engine;
    
    // Register QML types
    qmlRegisterType<RootStreamClient>("RootStream", 1, 0, "RootStreamClient");
    qmlRegisterType<PeerManager>("RootStream", 1, 0, "PeerManager");
    qmlRegisterType<SettingsManager>("RootStream", 1, 0, "SettingsManager");
    qmlRegisterType<LogManager>("RootStream", 1, 0, "LogManager");
    
    // Expose objects to QML
    engine.rootContext()->setContextProperty("rootStreamClient", &client);
    engine.rootContext()->setContextProperty("peerManager", &peerManager);
    engine.rootContext()->setContextProperty("settingsManager", &settingsManager);
    engine.rootContext()->setContextProperty("logManager", &logManager);
    
    // Load main QML file
    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection
    );
    engine.load(url);
    
    // Auto-connect if specified
    if (parser.isSet(autoConnectOption)) {
        QString code = parser.value(autoConnectOption);
        QMetaObject::invokeMethod(&client, "connectToPeer", Qt::QueuedConnection,
                                Q_ARG(QString, code));
    }
    
    return app.exec();
}
