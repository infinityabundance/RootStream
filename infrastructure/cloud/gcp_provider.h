#ifndef GCP_PROVIDER_H
#define GCP_PROVIDER_H

#include "cloud_provider.h"
#include <string>
#include <map>

/**
 * GCPProvider - Google Cloud Platform implementation of CloudProvider
 * Supports Compute Engine, Cloud Storage, Cloud SQL, and Cloud Monitoring
 */
class GCPProvider : public CloudProvider {
private:
    std::string projectId;
    std::string zone;
    std::string region;
    bool initialized;
    
    int executeGCloudCommand(const std::string &command,
                            const std::map<std::string, std::string> &params);
    
public:
    GCPProvider();
    ~GCPProvider() override;
    
    int init(const std::string &project, 
            const std::string &gceZone,
            const std::string &gceRegion);
    
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
    
    ProviderType getProviderType() const override { return GCP; }
    
    // GCP-specific methods
    int createFirewallRule(const std::string &ruleName,
                          const std::string &protocol,
                          int port);
    int createBucket(const std::string &bucketName);
};

#endif // GCP_PROVIDER_H
