/* SettingsManager Implementation */
#include "settingsmanager.h"
#include <QSettings>
#include <QDebug>

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
    , m_bitrate(10000000) // 10 Mbps default
{
}

void SettingsManager::load()
{
    QSettings settings("RootStream", "KDE-Client");
    
    m_codec = settings.value("codec", "h264").toString();
    m_bitrate = settings.value("bitrate", 10000000).toInt();
    
    emit codecChanged();
    emit bitrateChanged();
    
    qInfo() << "Loaded settings: codec=" << m_codec << "bitrate=" << m_bitrate;
}

void SettingsManager::save()
{
    QSettings settings("RootStream", "KDE-Client");
    
    settings.setValue("codec", m_codec);
    settings.setValue("bitrate", m_bitrate);
    
    settings.sync();
    
    qInfo() << "Saved settings";
}

void SettingsManager::setCodec(const QString &codec)
{
    if (m_codec != codec) {
        m_codec = codec;
        emit codecChanged();
    }
}

void SettingsManager::setBitrate(int bitrate)
{
    if (m_bitrate != bitrate) {
        m_bitrate = bitrate;
        emit bitrateChanged();
    }
}
