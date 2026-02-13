#ifndef PERFORMANCE_LOGGER_H
#define PERFORMANCE_LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QJsonArray>
#include "metrics_types.h"

class PerformanceLogger : public QObject {
    Q_OBJECT
    
public:
    explicit PerformanceLogger(QObject* parent = nullptr);
    ~PerformanceLogger();
    
    // Initialize logger
    bool init(const QString& filename);
    
    // Log snapshot to CSV
    bool logSnapshotCSV(const metrics_snapshot_t& metrics);
    
    // Log snapshot to JSON
    bool logSnapshotJSON(const metrics_snapshot_t& metrics);
    
    // Export and close
    bool exportJSON(const QString& output_file);
    bool finalize();
    
    // Enable/disable logging
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    
signals:
    void logError(const QString& error);
    
private:
    QFile m_csvFile;
    QTextStream m_csvStream;
    QJsonArray m_jsonArray;
    QString m_filename;
    bool m_enabled;
    bool m_csvHeaderWritten;
    uint32_t m_sampleCount;
};

#endif // PERFORMANCE_LOGGER_H
