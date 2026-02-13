#ifndef AZURE_PROVIDER_H
#define AZURE_PROVIDER_H

#include "cloud_provider.h"
#include <string>
#include <map>

/**
 * AzureProvider - Microsoft Azure implementation of CloudProvider
 * Supports Virtual Machines, Blob Storage, SQL Database, and Application Insights
 */
class AzureProvider : public CloudProvider {
private:
    std::string subscriptionId;
    std::string resourceGroup;
    std::string location;
    bool initialized;
    
    int executeAzureCommand(const std::string &command,
                           const std::map<std::string, std::string> &params);
    
public:
    AzureProvider();
    ~AzureProvider() override;
    
    int init(const std::string &subscription, 
            const std::string &resGroup,
            const std::string &loc);
    
    int createInstance(const InstanceConfig &config) override;
    int terminateInstance(const std::string &instanceId) override;
    int listInstances() override;
    
    int uploadFile(const std::string &bucket, 
                  const std::string &key,
                  const std::string &filePath) override;
    int downloadFile(const std::string &bucket,
                    const std::string &key,
                    const std::string &outputPath) override;
    
    DatabaseConnection* getDatabaseConnection() override;
    
    int createLoadBalancer(const LoadBalancerConfig &config) override;
    int registerTarget(const std::string &lbId,
                      const std::string &targetId) override;
    
    int publishMetric(const std::string &metricName,
                     float value) override;
    int logEvent(const std::string &logGroup,
                const std::string &event) override;
    
    ProviderType getProviderType() const override { return AZURE; }
    
    // Azure-specific methods
    int createVirtualNetwork(const std::string &vnetName,
                           const std::string &addressPrefix);
    int createStorageAccount(const std::string &accountName);
};

#endif // AZURE_PROVIDER_H
