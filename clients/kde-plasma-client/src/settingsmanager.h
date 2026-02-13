/* SettingsManager - Settings Persistence */
#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QString>

class SettingsManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString codec READ getCodec WRITE setCodec NOTIFY codecChanged)
    Q_PROPERTY(int bitrate READ getBitrate WRITE setBitrate NOTIFY bitrateChanged)
    
public:
    explicit SettingsManager(QObject *parent = nullptr);
    
    Q_INVOKABLE void load();
    Q_INVOKABLE void save();
    
    QString getCodec() const { return m_codec; }
    void setCodec(const QString &codec);
    bool hasCodec() const { return !m_codec.isEmpty(); }
    
    int getBitrate() const { return m_bitrate; }
    void setBitrate(int bitrate);
    bool hasBitrate() const { return m_bitrate > 0; }
    
signals:
    void codecChanged();
    void bitrateChanged();
    
private:
    QString m_codec;
    int m_bitrate;
};

#endif
