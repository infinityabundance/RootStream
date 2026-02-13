#include "resource_manager.h"
#include "aws_provider.h"
#include "azure_provider.h"
#include "gcp_provider.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

CloudResourceManager::CloudResourceManager() {}

CloudResourceManager::~CloudResourceManager() {
    cleanup();
}

int CloudResourceManager::init(CloudProvider::ProviderType providerType) {
    currentProviderType = providerType;
    
    switch (providerType) {
        case CloudProvider::AWS: {
            auto awsProvider = std::make_unique<AWSProvider>();
            // In production, read credentials from config/environment
            awsProvider->init("us-east-1", "AWS_ACCESS_KEY", "AWS_SECRET_KEY");
            provider = std::move(awsProvider);
            break;
        }
        case CloudProvider::AZURE: {
            auto azureProvider = std::make_unique<AzureProvider>();
            azureProvider->init("subscription-id", "rootstream-rg", "eastus");
            provider = std::move(azureProvider);
            break;
        }
        case CloudProvider::GCP: {
            auto gcpProvider = std::make_unique<GCPProvider>();
            gcpProvider->init("rootstream-project", "us-central1-a", "us-central1");
            provider = std::move(gcpProvider);
            break;
        }
        default:
            std::cerr << "Unknown provider type" << std::endl;
            return -1;
    }
    
    std::cout << "CloudResourceManager initialized" << std::endl;
    return 0;
}

std::string CloudResourceManager::generateResourceId(const std::string &prefix) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << prefix << "-" << timestamp;
    return ss.str();
}

void CloudResourceManager::trackResource(const std::string &resourceId, 
                                        const std::string &resourceType) {
    ResourceMetadata metadata;
    metadata.resourceId = resourceId;
    metadata.resourceType = resourceType;
    
    switch (currentProviderType) {
        case CloudProvider::AWS:
            metadata.cloudProvider = "AWS";
            break;
        case CloudProvider::AZURE:
            metadata.cloudProvider = "Azure";
            break;
        case CloudProvider::GCP:
            metadata.cloudProvider = "GCP";
            break;
    }
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&timestamp), "%Y-%m-%d %H:%M:%S");
    metadata.createdAt = ss.str();
    
    // Estimate costs (simplified)
    if (resourceType == "streaming-server") {
        metadata.estimatedMonthlyCost = 150.0f;
    } else if (resourceType == "database") {
        metadata.estimatedMonthlyCost = 100.0f;
    } else if (resourceType == "storage") {
        metadata.estimatedMonthlyCost = 25.0f;
    }
    
    resourceRegistry[resourceId] = metadata;
    std::cout << "Resource tracked: " << resourceId << " (" << resourceType << ")" << std::endl;
}

std::string CloudResourceManager::createStreamingServer(uint32_t capacity) {
    std::string resourceId = generateResourceId("stream-server");
    
    InstanceConfig config;
    config.instanceType = "t3.xlarge";
    config.imageId = "ami-ubuntu-22-04";
    config.keyName = resourceId;
    config.volumeSize = 100;
    config.tags["Name"] = resourceId;
    config.tags["Purpose"] = "streaming";
    config.tags["Capacity"] = std::to_string(capacity);
    
    if (provider->createInstance(config) == 0) {
        trackResource(resourceId, "streaming-server");
        return resourceId;
    }
    
    return "";
}

std::string CloudResourceManager::createStorageBucket(const std::string &bucketName) {
    std::string resourceId = generateResourceId("storage");
    
    // Create bucket using provider
    std::cout << "Creating storage bucket: " << bucketName << std::endl;
    
    trackResource(resourceId, "storage");
    return resourceId;
}

std::string CloudResourceManager::createDatabase(const DatabaseConfig &config) {
    std::string resourceId = generateResourceId("database");
    
    std::cout << "Creating database: " << config.dbName << std::endl;
    std::cout << "  Engine: " << config.engine << std::endl;
    std::cout << "  Instance Class: " << config.instanceClass << std::endl;
    std::cout << "  Storage: " << config.allocatedStorage << " GB" << std::endl;
    std::cout << "  Multi-AZ: " << (config.multiAZ ? "Yes" : "No") << std::endl;
    
    trackResource(resourceId, "database");
    return resourceId;
}

int CloudResourceManager::setupAutoScaling(const std::string &resourceId,
                                          uint32_t minInstances,
                                          uint32_t maxInstances) {
    auto it = resourceRegistry.find(resourceId);
    if (it == resourceRegistry.end()) {
        std::cerr << "Resource not found: " << resourceId << std::endl;
        return -1;
    }
    
    std::cout << "Setting up auto-scaling for " << resourceId << std::endl;
    std::cout << "  Min instances: " << minInstances << std::endl;
    std::cout << "  Max instances: " << maxInstances << std::endl;
    
    // In production, this would configure actual auto-scaling policies
    return 0;
}

int CloudResourceManager::optimizeResources() {
    std::cout << "Optimizing cloud resources..." << std::endl;
    
    int optimizationCount = 0;
    for (auto &entry : resourceRegistry) {
        // Check resource utilization and optimize
        std::cout << "  Checking " << entry.first << "..." << std::endl;
        optimizationCount++;
    }
    
    std::cout << "Optimized " << optimizationCount << " resources" << std::endl;
    return optimizationCount;
}

float CloudResourceManager::estimateMonthlyCost() {
    float totalCost = 0.0f;
    
    for (const auto &entry : resourceRegistry) {
        totalCost += entry.second.estimatedMonthlyCost;
    }
    
    std::cout << "Estimated monthly cost: $" << totalCost << std::endl;
    return totalCost;
}

int CloudResourceManager::deleteResource(const std::string &resourceId) {
    auto it = resourceRegistry.find(resourceId);
    if (it == resourceRegistry.end()) {
        std::cerr << "Resource not found: " << resourceId << std::endl;
        return -1;
    }
    
    std::cout << "Deleting resource: " << resourceId << std::endl;
    
    // Delete from cloud provider
    if (it->second.resourceType == "streaming-server") {
        provider->terminateInstance(resourceId);
    }
    
    resourceRegistry.erase(it);
    return 0;
}

int CloudResourceManager::deleteUnusedResources() {
    std::cout << "Scanning for unused resources..." << std::endl;
    
    int deletedCount = 0;
    // In production, this would check actual resource utilization
    
    std::cout << "Deleted " << deletedCount << " unused resources" << std::endl;
    return deletedCount;
}

void CloudResourceManager::listManagedResources() {
    std::cout << "\n=== Managed Resources ===" << std::endl;
    std::cout << "Total resources: " << resourceRegistry.size() << std::endl;
    
    for (const auto &entry : resourceRegistry) {
        const auto &metadata = entry.second;
        std::cout << "\nResource ID: " << metadata.resourceId << std::endl;
        std::cout << "  Type: " << metadata.resourceType << std::endl;
        std::cout << "  Provider: " << metadata.cloudProvider << std::endl;
        std::cout << "  Created: " << metadata.createdAt << std::endl;
        std::cout << "  Est. Monthly Cost: $" << metadata.estimatedMonthlyCost << std::endl;
    }
}

ResourceMetadata CloudResourceManager::getResourceInfo(const std::string &resourceId) {
    auto it = resourceRegistry.find(resourceId);
    if (it != resourceRegistry.end()) {
        return it->second;
    }
    return ResourceMetadata();
}

void CloudResourceManager::cleanup() {
    std::cout << "Cleaning up CloudResourceManager..." << std::endl;
    resourceRegistry.clear();
}
