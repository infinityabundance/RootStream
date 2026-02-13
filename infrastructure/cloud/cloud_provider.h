#ifndef CLOUD_PROVIDER_H
#define CLOUD_PROVIDER_H

#include <string>
#include <map>
#include <memory>
#include <vector>

// Forward declarations
struct InstanceConfig;
struct LoadBalancerConfig;
struct DatabaseConnection;

/**
 * CloudProvider - Abstract base class for cloud provider implementations
 * Supports AWS, Azure, and GCP with unified interface
 */
class CloudProvider {
public:
    enum ProviderType {
        AWS,
        AZURE,
        GCP
    };
    
    virtual ~CloudProvider() = default;
    
    // VM Instance management
    virtual int createInstance(const InstanceConfig &config) = 0;
    virtual int terminateInstance(const std::string &instanceId) = 0;
    virtual int listInstances() = 0;
    
    // Storage
    virtual int uploadFile(const std::string &bucket, 
                          const std::string &key,
                          const std::string &filePath) = 0;
    virtual int downloadFile(const std::string &bucket,
                            const std::string &key,
                            const std::string &outputPath) = 0;
    
    // Database
    virtual DatabaseConnection* getDatabaseConnection() = 0;
    
    // Load Balancer
    virtual int createLoadBalancer(const LoadBalancerConfig &config) = 0;
    virtual int registerTarget(const std::string &lbId,
                              const std::string &targetId) = 0;
    
    // Monitoring & Logging
    virtual int publishMetric(const std::string &metricName,
                            float value) = 0;
    virtual int logEvent(const std::string &logGroup,
                        const std::string &event) = 0;
    
    // Provider type getter
    virtual ProviderType getProviderType() const = 0;
};

// Configuration structures
struct InstanceConfig {
    std::string instanceType;
    std::string imageId;
    std::string keyName;
    std::string subnetId;
    std::map<std::string, std::string> tags;
    int volumeSize;
};

struct LoadBalancerConfig {
    std::string name;
    std::string type;  // "application" or "network"
    bool internal;
    std::vector<std::string> subnets;
    std::map<std::string, std::string> tags;
};

struct DatabaseConnection {
    std::string endpoint;
    int port;
    std::string username;
    std::string database;
    bool isConnected;
};

#endif // CLOUD_PROVIDER_H
