/* LogManager - AI Logging Integration */
#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>
#include <QString>

class LogManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    
public:
    explicit LogManager(QObject *parent = nullptr);
    
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled);
    
    Q_INVOKABLE QString getLogs();
    Q_INVOKABLE void clearLogs();
    
signals:
    void enabledChanged();
    void logAdded(const QString &message);
    
private:
    bool m_enabled;
};

#endif
