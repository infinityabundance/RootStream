#include "metrics.h"
#include <iostream>
#include <sstream>
#include <iomanip>

MetricsCollector::MetricsCollector() : initialized(false) {}

MetricsCollector::~MetricsCollector() {
    cleanup();
}

int MetricsCollector::init() {
    std::cout << "Initializing MetricsCollector..." << std::endl;
    initialized = true;
    return 0;
}

void MetricsCollector::incrementCounter(const std::string &name, float delta) {
    if (!initialized) return;
    
    auto it = metrics.find(name);
    if (it != metrics.end()) {
        it->second.value += delta;
        it->second.timestamp = std::chrono::system_clock::now();
    } else {
        Metric metric;
        metric.name = name;
        metric.type = COUNTER;
        metric.value = delta;
        metric.timestamp = std::chrono::system_clock::now();
        metrics[name] = metric;
    }
}

void MetricsCollector::setCounter(const std::string &name, float value) {
    if (!initialized) return;
    
    Metric metric;
    metric.name = name;
    metric.type = COUNTER;
    metric.value = value;
    metric.timestamp = std::chrono::system_clock::now();
    metrics[name] = metric;
}

void MetricsCollector::setGauge(const std::string &name, float value) {
    if (!initialized) return;
    
    Metric metric;
    metric.name = name;
    metric.type = GAUGE;
    metric.value = value;
    metric.timestamp = std::chrono::system_clock::now();
    metrics[name] = metric;
}

void MetricsCollector::incrementGauge(const std::string &name, float delta) {
    if (!initialized) return;
    
    auto it = metrics.find(name);
    if (it != metrics.end()) {
        it->second.value += delta;
        it->second.timestamp = std::chrono::system_clock::now();
    } else {
        setGauge(name, delta);
    }
}

void MetricsCollector::decrementGauge(const std::string &name, float delta) {
    incrementGauge(name, -delta);
}

void MetricsCollector::observeHistogram(const std::string &name, float value) {
    if (!initialized) return;
    
    // Simplified histogram - just stores current value
    // In production, would maintain buckets and calculate percentiles
    Metric metric;
    metric.name = name;
    metric.type = HISTOGRAM;
    metric.value = value;
    metric.timestamp = std::chrono::system_clock::now();
    metrics[name] = metric;
}

MetricsCollector::Metric MetricsCollector::getMetric(const std::string &name) {
    auto it = metrics.find(name);
    if (it != metrics.end()) {
        return it->second;
    }
    return Metric();
}

std::vector<MetricsCollector::Metric> MetricsCollector::getAllMetrics() {
    std::vector<Metric> result;
    for (const auto &entry : metrics) {
        result.push_back(entry.second);
    }
    return result;
}

std::string MetricsCollector::exportPrometheus() {
    std::stringstream output;
    
    for (const auto &entry : metrics) {
        const Metric &metric = entry.second;
        
        // Type hint
        std::string typeStr;
        switch (metric.type) {
            case COUNTER: typeStr = "counter"; break;
            case GAUGE: typeStr = "gauge"; break;
            case HISTOGRAM: typeStr = "histogram"; break;
        }
        
        output << "# HELP " << metric.name << " " << metric.name << "\n";
        output << "# TYPE " << metric.name << " " << typeStr << "\n";
        
        // Metric line with labels
        output << metric.name;
        if (!metric.labels.empty()) {
            output << "{";
            bool first = true;
            for (const auto &label : metric.labels) {
                if (!first) output << ",";
                output << label.first << "=\"" << label.second << "\"";
                first = false;
            }
            output << "}";
        }
        output << " " << metric.value << "\n";
    }
    
    return output.str();
}

std::string MetricsCollector::exportJSON() {
    std::stringstream output;
    output << "{\n";
    output << "  \"metrics\": [\n";
    
    bool first = true;
    for (const auto &entry : metrics) {
        if (!first) output << ",\n";
        first = false;
        
        const Metric &metric = entry.second;
        output << "    {\n";
        output << "      \"name\": \"" << metric.name << "\",\n";
        output << "      \"type\": \"";
        
        switch (metric.type) {
            case COUNTER: output << "counter"; break;
            case GAUGE: output << "gauge"; break;
            case HISTOGRAM: output << "histogram"; break;
        }
        
        output << "\",\n";
        output << "      \"value\": " << metric.value << ",\n";
        output << "      \"labels\": {";
        
        bool firstLabel = true;
        for (const auto &label : metric.labels) {
            if (!firstLabel) output << ", ";
            output << "\"" << label.first << "\": \"" << label.second << "\"";
            firstLabel = false;
        }
        
        output << "}\n";
        output << "    }";
    }
    
    output << "\n  ]\n";
    output << "}\n";
    
    return output.str();
}

void MetricsCollector::addLabel(const std::string &metricName, 
                                const std::string &labelKey,
                                const std::string &labelValue) {
    auto it = metrics.find(metricName);
    if (it != metrics.end()) {
        it->second.labels[labelKey] = labelValue;
    }
}

void MetricsCollector::cleanup() {
    std::cout << "Cleaning up MetricsCollector..." << std::endl;
    metrics.clear();
}
