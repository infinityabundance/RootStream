#ifndef METRICS_H
#define METRICS_H

#include <string>
#include <map>
#include <chrono>
#include <vector>

/**
 * MetricsCollector - Collect and export application metrics
 * Supports various metric types (counters, gauges, histograms)
 */
class MetricsCollector {
public:
    enum MetricType {
        COUNTER,    // Monotonically increasing value
        GAUGE,      // Value that can go up or down
        HISTOGRAM   // Distribution of values
    };
    
    struct Metric {
        std::string name;
        MetricType type;
        float value;
        std::map<std::string, std::string> labels;
        std::chrono::system_clock::time_point timestamp;
    };
    
private:
    std::map<std::string, Metric> metrics;
    bool initialized;
    
public:
    MetricsCollector();
    ~MetricsCollector();
    
    int init();
    
    // Counter operations
    void incrementCounter(const std::string &name, float delta = 1.0f);
    void setCounter(const std::string &name, float value);
    
    // Gauge operations
    void setGauge(const std::string &name, float value);
    void incrementGauge(const std::string &name, float delta);
    void decrementGauge(const std::string &name, float delta);
    
    // Histogram operations
    void observeHistogram(const std::string &name, float value);
    
    // Get metrics
    Metric getMetric(const std::string &name);
    std::vector<Metric> getAllMetrics();
    
    // Export formats
    std::string exportPrometheus();
    std::string exportJSON();
    
    // Labels
    void addLabel(const std::string &metricName, 
                 const std::string &labelKey,
                 const std::string &labelValue);
    
    void cleanup();
};

#endif // METRICS_H
