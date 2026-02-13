#include <QtTest/QtTest>
#include <QSignalSpy>
#include "../src/metrics/performance_aggregator.h"
#include "../src/metrics/hud_renderer.h"
#include "../src/metrics/performance_logger.h"
#include "../src/metrics/alert_system.h"

extern "C" {
#include "../src/metrics/frame_rate_counter.h"
#include "../src/metrics/cpu_monitor.h"
#include "../src/metrics/memory_monitor.h"
#include "../src/metrics/gpu_monitor.h"
}

class TestMetrics : public QObject {
    Q_OBJECT
    
private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Frame rate counter tests
    void testFrameRateCounter();
    void testFrameRateCounterStats();
    void testFrameDropDetection();
    
    // CPU monitor tests
    void testCPUMonitor();
    void testCPUTemperature();
    
    // Memory monitor tests
    void testMemoryMonitor();
    
    // GPU monitor tests
    void testGPUMonitor();
    
    // Performance aggregator tests
    void testPerformanceAggregator();
    void testMetricsSignals();
    void testAnomalyDetection();
    
    // HUD renderer tests
    void testHUDRenderer();
    void testHUDConfiguration();
    
    // Performance logger tests
    void testPerformanceLoggerCSV();
    void testPerformanceLoggerJSON();
    
    // Alert system tests
    void testAlertSystem();
    void testAlertThresholds();
    void testAlertDebouncing();
};

void TestMetrics::initTestCase() {
    qDebug() << "Starting metrics tests...";
}

void TestMetrics::cleanupTestCase() {
    qDebug() << "Metrics tests completed.";
}

void TestMetrics::testFrameRateCounter() {
    frame_rate_counter_t* counter = frame_rate_counter_init();
    QVERIFY(counter != nullptr);
    
    // Simulate 60 frames
    for (int i = 0; i < 60; i++) {
        frame_rate_counter_record_frame(counter);
        QTest::qWait(16); // ~60 FPS
    }
    
    uint32_t fps = frame_rate_counter_get_fps(counter);
    qDebug() << "Measured FPS:" << fps;
    
    // Allow some variance
    QVERIFY(fps >= 50 && fps <= 70);
    
    frame_rate_counter_cleanup(counter);
}

void TestMetrics::testFrameRateCounterStats() {
    frame_rate_counter_t* counter = frame_rate_counter_init();
    QVERIFY(counter != nullptr);
    
    // Simulate frames
    for (int i = 0; i < 100; i++) {
        frame_rate_counter_record_frame(counter);
        QTest::qWait(16);
    }
    
    frame_rate_metrics_t stats;
    frame_rate_counter_get_stats(counter, &stats);
    
    QVERIFY(stats.total_frames == 100);
    QVERIFY(stats.fps > 0);
    QVERIFY(stats.min_frame_time_ms > 0);
    QVERIFY(stats.max_frame_time_ms > stats.min_frame_time_ms);
    QVERIFY(stats.avg_frame_time_ms > 0);
    
    qDebug() << "Frame stats - FPS:" << stats.fps 
             << "Avg:" << stats.avg_frame_time_ms << "ms"
             << "Min:" << stats.min_frame_time_ms << "ms"
             << "Max:" << stats.max_frame_time_ms << "ms";
    
    frame_rate_counter_cleanup(counter);
}

void TestMetrics::testFrameDropDetection() {
    frame_rate_counter_t* counter = frame_rate_counter_init();
    QVERIFY(counter != nullptr);
    
    // Simulate normal frames
    for (int i = 0; i < 10; i++) {
        frame_rate_counter_record_frame(counter);
        QTest::qWait(16);
    }
    
    // Simulate a dropped frame (long delay)
    QTest::qWait(100);
    frame_rate_counter_record_frame(counter);
    
    uint32_t drops = frame_rate_counter_get_dropped_frames(counter);
    qDebug() << "Detected dropped frames:" << drops;
    QVERIFY(drops > 0);
    
    frame_rate_counter_cleanup(counter);
}

void TestMetrics::testCPUMonitor() {
    cpu_monitor_t* monitor = cpu_monitor_init();
    QVERIFY(monitor != nullptr);
    
    cpu_monitor_update(monitor);
    
    uint8_t usage = cpu_monitor_get_usage(monitor);
    float load = cpu_monitor_get_load_average(monitor);
    
    qDebug() << "CPU usage:" << usage << "%";
    qDebug() << "Load average:" << load;
    
    QVERIFY(usage <= 100);
    QVERIFY(load >= 0.0f);
    
    cpu_metrics_t stats;
    cpu_monitor_get_stats(monitor, &stats);
    
    QVERIFY(stats.num_cores > 0);
    QVERIFY(stats.cpu_usage_percent <= 100);
    
    cpu_monitor_cleanup(monitor);
}

void TestMetrics::testCPUTemperature() {
    cpu_monitor_t* monitor = cpu_monitor_init();
    QVERIFY(monitor != nullptr);
    
    cpu_monitor_update(monitor);
    
    uint8_t temp = cpu_monitor_get_temperature(monitor);
    bool throttling = cpu_monitor_is_thermal_throttling(monitor);
    
    qDebug() << "CPU temperature:" << temp << "°C";
    qDebug() << "Thermal throttling:" << throttling;
    
    // Temperature should be reasonable (0-120°C range)
    QVERIFY(temp == 0 || (temp > 0 && temp < 120));
    
    cpu_monitor_cleanup(monitor);
}

void TestMetrics::testMemoryMonitor() {
    memory_monitor_t* monitor = memory_monitor_init();
    QVERIFY(monitor != nullptr);
    
    memory_monitor_update(monitor);
    
    uint32_t total = memory_monitor_get_ram_total_mb(monitor);
    uint32_t used = memory_monitor_get_ram_used_mb(monitor);
    uint8_t percent = memory_monitor_get_ram_usage_percent(monitor);
    
    qDebug() << "RAM:" << used << "/" << total << "MB (" << percent << "%)";
    
    QVERIFY(total > 0);
    QVERIFY(used <= total);
    QVERIFY(percent <= 100);
    
    memory_metrics_t stats;
    memory_monitor_get_stats(monitor, &stats);
    
    QVERIFY(stats.ram_total_mb > 0);
    QVERIFY(stats.ram_used_mb <= stats.ram_total_mb);
    
    memory_monitor_cleanup(monitor);
}

void TestMetrics::testGPUMonitor() {
    gpu_monitor_t* monitor = gpu_monitor_init();
    QVERIFY(monitor != nullptr);
    
    gpu_monitor_update(monitor);
    
    uint32_t vram_total = gpu_monitor_get_vram_total_mb(monitor);
    uint8_t util = gpu_monitor_get_utilization(monitor);
    uint8_t temp = gpu_monitor_get_temperature(monitor);
    
    qDebug() << "GPU - VRAM:" << vram_total << "MB";
    qDebug() << "GPU - Utilization:" << util << "%";
    qDebug() << "GPU - Temperature:" << temp << "°C";
    
    QVERIFY(util <= 100);
    
    gpu_metrics_t stats;
    gpu_monitor_get_stats(monitor, &stats);
    
    qDebug() << "GPU Model:" << stats.gpu_model;
    
    gpu_monitor_cleanup(monitor);
}

void TestMetrics::testPerformanceAggregator() {
    PerformanceAggregator aggregator;
    QVERIFY(aggregator.init());
    
    // Record some frames
    for (int i = 0; i < 10; i++) {
        aggregator.recordFrame();
        QTest::qWait(16);
    }
    
    // Record some metrics
    aggregator.recordNetworkLatency(50);
    aggregator.recordInput(10);
    aggregator.recordAVSyncOffset(5);
    
    QTest::qWait(1100); // Wait for update timer
    
    metrics_snapshot_t snapshot = aggregator.getLatestSnapshot();
    
    QVERIFY(snapshot.timestamp_us > 0);
    QVERIFY(snapshot.fps.total_frames > 0);
    
    qDebug() << "Snapshot - FPS:" << snapshot.fps.fps;
    qDebug() << "Snapshot - RTT:" << snapshot.network.rtt_ms << "ms";
    qDebug() << "Snapshot - CPU:" << snapshot.cpu.cpu_usage_percent << "%";
}

void TestMetrics::testMetricsSignals() {
    PerformanceAggregator aggregator;
    QVERIFY(aggregator.init());
    
    QSignalSpy spy(&aggregator, &PerformanceAggregator::metricsUpdated);
    
    // Wait for at least one update
    QTest::qWait(1100);
    
    QVERIFY(spy.count() >= 1);
    qDebug() << "Received" << spy.count() << "metrics updates";
}

void TestMetrics::testAnomalyDetection() {
    PerformanceAggregator aggregator;
    QVERIFY(aggregator.init());
    
    // Simulate high latency
    aggregator.recordNetworkLatency(150);
    QTest::qWait(1100);
    
    bool highLatency = aggregator.detectHighLatency();
    QVERIFY(highLatency);
    
    qDebug() << "High latency detected:" << highLatency;
}

void TestMetrics::testHUDRenderer() {
    HUDRenderer hud;
    QVERIFY(hud.init(1920, 1080));
    
    QVERIFY(hud.isHUDVisible());
    
    hud.setHUDVisible(false);
    QVERIFY(!hud.isHUDVisible());
    
    hud.setHUDVisible(true);
    QVERIFY(hud.isHUDVisible());
}

void TestMetrics::testHUDConfiguration() {
    HUDRenderer hud;
    QVERIFY(hud.init(1920, 1080));
    
    hud.setHUDOpacity(0.5f);
    hud.setShowFPS(true);
    hud.setShowLatency(true);
    hud.setShowNetwork(false);
    hud.setShowResources(true);
    
    // Verify configuration doesn't crash
    QVERIFY(true);
}

void TestMetrics::testPerformanceLoggerCSV() {
    PerformanceLogger logger;
    QString filename = "/tmp/test_metrics.csv";
    
    QVERIFY(logger.init(filename));
    
    // Create test snapshot
    metrics_snapshot_t snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.timestamp_us = 1000000;
    snapshot.fps.fps = 60;
    snapshot.network.rtt_ms = 50;
    
    QVERIFY(logger.logSnapshotCSV(snapshot));
    QVERIFY(logger.finalize());
    
    // Verify file exists
    QFile file(filename);
    QVERIFY(file.exists());
    QVERIFY(file.size() > 0);
    
    qDebug() << "CSV file size:" << file.size() << "bytes";
    
    // Cleanup
    file.remove();
}

void TestMetrics::testPerformanceLoggerJSON() {
    PerformanceLogger logger;
    logger.setEnabled(true);
    
    // Create test snapshot
    metrics_snapshot_t snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.timestamp_us = 1000000;
    snapshot.fps.fps = 60;
    snapshot.network.rtt_ms = 50;
    
    QVERIFY(logger.logSnapshotJSON(snapshot));
    
    QString filename = "/tmp/test_metrics.json";
    QVERIFY(logger.exportJSON(filename));
    
    // Verify file exists
    QFile file(filename);
    QVERIFY(file.exists());
    QVERIFY(file.size() > 0);
    
    qDebug() << "JSON file size:" << file.size() << "bytes";
    
    // Cleanup
    file.remove();
}

void TestMetrics::testAlertSystem() {
    AlertSystem alerts;
    QVERIFY(alerts.init());
    
    QSignalSpy fpsDropSpy(&alerts, &AlertSystem::alertFPSDrop);
    QSignalSpy latencySpy(&alerts, &AlertSystem::alertHighLatency);
    
    // Create metrics that should trigger alerts
    metrics_snapshot_t snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.fps.fps = 20; // Below default threshold of 30
    snapshot.network.rtt_ms = 150; // Above default threshold of 100
    
    alerts.checkMetrics(snapshot);
    
    QVERIFY(fpsDropSpy.count() > 0);
    QVERIFY(latencySpy.count() > 0);
    
    qDebug() << "FPS drop alerts:" << fpsDropSpy.count();
    qDebug() << "Latency alerts:" << latencySpy.count();
}

void TestMetrics::testAlertThresholds() {
    AlertSystem alerts;
    QVERIFY(alerts.init());
    
    // Set custom thresholds
    alerts.setFPSDropThreshold(45);
    alerts.setLatencyThreshold(80);
    alerts.setThermalThreshold(90);
    
    metrics_snapshot_t snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.fps.fps = 50; // Above new threshold, should not alert
    
    QSignalSpy fpsDropSpy(&alerts, &AlertSystem::alertFPSDrop);
    alerts.checkMetrics(snapshot);
    
    QVERIFY(fpsDropSpy.count() == 0);
}

void TestMetrics::testAlertDebouncing() {
    AlertSystem alerts;
    QVERIFY(alerts.init());
    
    QSignalSpy fpsDropSpy(&alerts, &AlertSystem::alertFPSDrop);
    
    metrics_snapshot_t snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.fps.fps = 20;
    
    // First alert should trigger
    alerts.checkMetrics(snapshot);
    QVERIFY(fpsDropSpy.count() == 1);
    
    // Immediate second alert should be debounced
    alerts.checkMetrics(snapshot);
    QVERIFY(fpsDropSpy.count() == 1);
    
    qDebug() << "Alert debouncing working correctly";
}

QTEST_MAIN(TestMetrics)
#include "test_metrics.moc"
