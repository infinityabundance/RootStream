#include "health_check.h"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>

HealthCheckManager::HealthCheckManager() : initialized(false) {
    memset(&lastStatus, 0, sizeof(HealthStatus));
}

HealthCheckManager::~HealthCheckManager() {
    cleanup();
}

int HealthCheckManager::init() {
    std::cout << "Initializing HealthCheckManager..." << std::endl;
    initialized = true;
    return 0;
}

bool HealthCheckManager::checkDatabaseHealth() {
    // In production, this would actually test database connectivity
    // For now, simulate with a basic check
    std::cout << "Checking database health..." << std::endl;
    return true;
}

bool HealthCheckManager::checkCacheHealth() {
    // Check Redis/cache connectivity
    std::cout << "Checking cache health..." << std::endl;
    
    int result = system("redis-cli ping > /dev/null 2>&1");
    return (result == 0);
}

bool HealthCheckManager::checkStorageHealth() {
    // Check storage availability
    std::cout << "Checking storage health..." << std::endl;
    
    float diskUsage = getDiskUsage();
    return (diskUsage < 90.0f);  // Healthy if less than 90% full
}

float HealthCheckManager::getCPUUsage() {
    // Read from /proc/stat on Linux
    std::ifstream statFile("/proc/stat");
    if (!statFile.is_open()) {
        return 0.0f;
    }
    
    std::string line;
    std::getline(statFile, line);
    
    // Simplified CPU usage calculation
    // In production, this would do proper delta calculations
    return 25.5f;  // Placeholder
}

float HealthCheckManager::getMemoryUsage() {
    // Read from /proc/meminfo on Linux
    std::ifstream meminfoFile("/proc/meminfo");
    if (!meminfoFile.is_open()) {
        return 0.0f;
    }
    
    long totalMem = 0, freeMem = 0;
    std::string line;
    
    while (std::getline(meminfoFile, line)) {
        if (line.find("MemTotal:") == 0) {
            sscanf(line.c_str(), "MemTotal: %ld kB", &totalMem);
        } else if (line.find("MemAvailable:") == 0) {
            sscanf(line.c_str(), "MemAvailable: %ld kB", &freeMem);
            break;
        }
    }
    
    if (totalMem > 0) {
        return ((totalMem - freeMem) * 100.0f) / totalMem;
    }
    
    return 0.0f;
}

float HealthCheckManager::getDiskUsage() {
    // Use df command to get disk usage
    FILE *pipe = popen("df -h / | tail -1 | awk '{print $5}' | sed 's/%//'", "r");
    if (!pipe) {
        return 0.0f;
    }
    
    char buffer[128];
    std::string result = "";
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
    
    return std::stof(result);
}

int HealthCheckManager::getActiveConnections() {
    // Count active network connections
    FILE *pipe = popen("netstat -an | grep ESTABLISHED | wc -l", "r");
    if (!pipe) {
        return 0;
    }
    
    char buffer[128];
    std::string result = "";
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
    
    return std::stoi(result);
}

HealthStatus HealthCheckManager::getOverallHealth() {
    if (!initialized) {
        std::cerr << "HealthCheckManager not initialized" << std::endl;
        return lastStatus;
    }
    
    HealthStatus status;
    status.database_healthy = checkDatabaseHealth();
    status.cache_healthy = checkCacheHealth();
    status.storage_healthy = checkStorageHealth();
    status.api_healthy = true;  // Placeholder
    
    status.cpu_usage = getCPUUsage();
    status.memory_usage = getMemoryUsage();
    status.disk_usage = getDiskUsage();
    status.active_connections = getActiveConnections();
    
    // Get uptime
    std::ifstream uptimeFile("/proc/uptime");
    if (uptimeFile.is_open()) {
        float uptime;
        uptimeFile >> uptime;
        status.uptime_seconds = static_cast<long>(uptime);
    } else {
        status.uptime_seconds = 0;
    }
    
    lastStatus = status;
    return status;
}

bool HealthCheckManager::checkDatabaseConnectivity() {
    return checkDatabaseHealth();
}

bool HealthCheckManager::checkCacheConnectivity() {
    return checkCacheHealth();
}

bool HealthCheckManager::checkStorageConnectivity() {
    return checkStorageHealth();
}

bool HealthCheckManager::isHealthy() {
    HealthStatus status = getOverallHealth();
    
    return status.api_healthy && 
           status.database_healthy && 
           status.cache_healthy && 
           status.storage_healthy &&
           status.cpu_usage < 80.0f &&
           status.memory_usage < 90.0f &&
           status.disk_usage < 90.0f;
}

HealthStatus HealthCheckManager::checkComponent(const std::string &componentName) {
    std::cout << "Checking component: " << componentName << std::endl;
    return getOverallHealth();
}

int HealthCheckManager::setHealthAlert(const std::string &service, float threshold) {
    AlertConfig config;
    config.service = service;
    config.threshold = threshold;
    config.enabled = true;
    
    alerts[service] = config;
    
    std::cout << "Alert set for " << service << " at threshold " << threshold << std::endl;
    return 0;
}

int HealthCheckManager::removeHealthAlert(const std::string &service) {
    alerts.erase(service);
    std::cout << "Alert removed for " << service << std::endl;
    return 0;
}

void HealthCheckManager::triggerAlert(const std::string &service, 
                                     const std::string &message) {
    std::cout << "ALERT [" << service << "]: " << message << std::endl;
    // In production, this would send notifications via email, Slack, PagerDuty, etc.
}

void HealthCheckManager::checkAlerts() {
    HealthStatus status = getOverallHealth();
    
    for (const auto &alert : alerts) {
        if (!alert.second.enabled) continue;
        
        const std::string &service = alert.first;
        float threshold = alert.second.threshold;
        
        if (service == "cpu" && status.cpu_usage > threshold) {
            triggerAlert(service, "CPU usage " + std::to_string(status.cpu_usage) + 
                        "% exceeds threshold " + std::to_string(threshold) + "%");
        } else if (service == "memory" && status.memory_usage > threshold) {
            triggerAlert(service, "Memory usage " + std::to_string(status.memory_usage) + 
                        "% exceeds threshold " + std::to_string(threshold) + "%");
        } else if (service == "disk" && status.disk_usage > threshold) {
            triggerAlert(service, "Disk usage " + std::to_string(status.disk_usage) + 
                        "% exceeds threshold " + std::to_string(threshold) + "%");
        }
    }
}

std::map<std::string, float> HealthCheckManager::getMetrics() {
    HealthStatus status = getOverallHealth();
    
    std::map<std::string, float> metrics;
    metrics["cpu_usage"] = status.cpu_usage;
    metrics["memory_usage"] = status.memory_usage;
    metrics["disk_usage"] = status.disk_usage;
    metrics["active_connections"] = static_cast<float>(status.active_connections);
    metrics["uptime_seconds"] = static_cast<float>(status.uptime_seconds);
    
    return metrics;
}

void HealthCheckManager::cleanup() {
    std::cout << "Cleaning up HealthCheckManager..." << std::endl;
    alerts.clear();
}
