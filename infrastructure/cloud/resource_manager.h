#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include "cloud_provider.h"
#include <string>
#include <map>
#include <memory>

struct ResourceMetadata {
    std::string resourceId;
    std::string resourceType;
    std::string cloudProvider;
    std::string createdAt;
    std::map<std::string, std::string> tags;
    float estimatedMonthlyCost;
};

struct DatabaseConfig {
    std::string engine;
    std::string instanceClass;
    int allocatedStorage;
    std::string dbName;
    std::string username;
    std::string password;
    bool multiAZ;
};

/**
 * CloudResourceManager - High-level resource management across cloud providers
 * Handles resource tracking, auto-scaling, and cost optimization
 */
class CloudResourceManager {
private:
    std::unique_ptr<CloudProvider> provider;
    std::map<std::string, ResourceMetadata> resourceRegistry;
    CloudProvider::ProviderType currentProviderType;
    
    std::string generateResourceId(const std::string &prefix);
    void trackResource(const std::string &resourceId, 
                      const std::string &resourceType);
    
public:
    CloudResourceManager();
    ~CloudResourceManager();
    
    int init(CloudProvider::ProviderType providerType);
    
    // Resource creation with auto-tracking
    std::string createStreamingServer(uint32_t capacity);
    std::string createStorageBucket(const std::string &bucketName);
    std::string createDatabase(const DatabaseConfig &config);
    
    // Auto-scaling
    int setupAutoScaling(const std::string &resourceId,
                        uint32_t minInstances,
                        uint32_t maxInstances);
    
    // Cost optimization
    int optimizeResources();
    float estimateMonthlyCost();
    
    // Resource cleanup
    int deleteResource(const std::string &resourceId);
    int deleteUnusedResources();
    
    // Resource listing
    void listManagedResources();
    ResourceMetadata getResourceInfo(const std::string &resourceId);
    
    void cleanup();
};

#endif // RESOURCE_MANAGER_H
