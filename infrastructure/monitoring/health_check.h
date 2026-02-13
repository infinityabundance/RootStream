#ifndef HEALTH_CHECK_H
#define HEALTH_CHECK_H

#include <string>
#include <map>

/**
 * HealthCheckManager - System health monitoring and alerting
 * Monitors API, database, cache, storage, and system resources
 */
class HealthCheckManager {
public:
    struct HealthStatus {
        bool api_healthy;
        bool database_healthy;
        bool cache_healthy;
        bool storage_healthy;
        int active_connections;
        float cpu_usage;
        float memory_usage;
        float disk_usage;
        long uptime_seconds;
    };
    
    struct AlertConfig {
        std::string service;
        float threshold;
        bool enabled;
    };
    
private:
    bool initialized;
    std::map<std::string, AlertConfig> alerts;
    HealthStatus lastStatus;
    
    // Internal check methods
    bool checkDatabaseHealth();
    bool checkCacheHealth();
    bool checkStorageHealth();
    float getCPUUsage();
    float getMemoryUsage();
    float getDiskUsage();
    int getActiveConnections();
    
    void triggerAlert(const std::string &service, const std::string &message);
    
public:
    HealthCheckManager();
    ~HealthCheckManager();
    
    int init();
    
    // Health endpoints
    HealthStatus getOverallHealth();
    bool checkDatabaseConnectivity();
    bool checkCacheConnectivity();
    bool checkStorageConnectivity();
    bool isHealthy();
    
    // Individual component checks
    HealthStatus checkComponent(const std::string &componentName);
    
    // Alerting
    int setHealthAlert(const std::string &service, float threshold);
    int removeHealthAlert(const std::string &service);
    void checkAlerts();
    
    // Metrics
    std::map<std::string, float> getMetrics();
    
    void cleanup();
};

#endif // HEALTH_CHECK_H
