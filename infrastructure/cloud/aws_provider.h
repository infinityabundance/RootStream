#ifndef AWS_PROVIDER_H
#define AWS_PROVIDER_H

#include "cloud_provider.h"
#include <string>
#include <map>

/**
 * AWSProvider - AWS implementation of CloudProvider
 * Supports EC2, S3, RDS, CloudWatch, and ELB
 */
class AWSProvider : public CloudProvider {
private:
    std::string region;
    std::string accessKeyId;
    std::string secretAccessKey;
    bool initialized;
    
    // Helper methods
    int executeAWSCommand(const std::string &service, 
                         const std::string &command,
                         const std::map<std::string, std::string> &params);
    
public:
    AWSProvider();
    ~AWSProvider() override;
    
    // Initialization
    int init(const std::string &region, 
            const std::string &accessKey,
            const std::string &secretKey);
    
    // CloudProvider interface implementation
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
    
    ProviderType getProviderType() const override { return AWS; }
    
    // AWS-specific methods
    int createSecurityGroup(const std::string &groupName,
                           const std::string &description,
                           const std::string &vpcId);
    int authorizeSecurityGroupIngress(const std::string &groupId,
                                     int port,
                                     const std::string &protocol);
};

#endif // AWS_PROVIDER_H
