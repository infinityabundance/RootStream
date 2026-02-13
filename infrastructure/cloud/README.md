# Cloud Provider Abstraction Layer

This module provides a unified interface for cloud providers (AWS, Azure, GCP) to manage infrastructure resources for RootStream.

## Overview

The cloud provider abstraction layer allows RootStream to work seamlessly across multiple cloud platforms without code changes. It provides consistent interfaces for:

- **VM Instance Management**: Create, terminate, and list compute instances
- **Storage**: Upload and download files to/from cloud storage (S3, Azure Blob, GCS)
- **Database**: Connect to managed database services (RDS, Azure SQL, Cloud SQL)
- **Load Balancing**: Create and manage load balancers
- **Monitoring**: Publish metrics and log events

## Architecture

```
┌──────────────────────────────────────┐
│      Cloud Resource Manager          │
│   (High-level resource management)   │
└──────────────┬───────────────────────┘
               │
       ┌───────┴────────┐
       │                │
┌──────▼──────┐  ┌──────▼──────┐  ┌──────▼──────┐
│             │  │             │  │             │
│ AWS Provider│  │Azure Provider│ │ GCP Provider│
│             │  │             │  │             │
└─────────────┘  └─────────────┘  └─────────────┘
```

## Components

### 1. CloudProvider (Base Interface)

Abstract base class defining the common interface for all cloud providers.

**File**: `cloud_provider.h`

### 2. AWS Provider

Implementation for Amazon Web Services.

**Files**: `aws_provider.h`, `aws_provider.cpp`

**Features**:
- EC2 instance management
- S3 storage operations
- RDS database connections
- CloudWatch monitoring
- Elastic Load Balancing

### 3. Azure Provider

Implementation for Microsoft Azure.

**Files**: `azure_provider.h`, `azure_provider.cpp`

**Features**:
- Virtual Machine management
- Azure Blob Storage
- Azure SQL Database
- Application Insights
- Azure Load Balancer

### 4. GCP Provider

Implementation for Google Cloud Platform.

**Files**: `gcp_provider.h`, `gcp_provider.cpp`

**Features**:
- Compute Engine instances
- Cloud Storage
- Cloud SQL
- Cloud Monitoring
- Cloud Load Balancing

### 5. Resource Manager

High-level resource management with tracking and cost estimation.

**Files**: `resource_manager.h`, `resource_manager.cpp`

**Features**:
- Unified resource creation and management
- Cost tracking and estimation
- Auto-scaling configuration
- Resource cleanup and optimization

## Usage

### AWS Example

```cpp
#include "aws_provider.h"
#include "resource_manager.h"

// Initialize AWS provider
AWSProvider awsProvider;
awsProvider.init("us-east-1", "ACCESS_KEY", "SECRET_KEY");

// Create an instance
InstanceConfig config;
config.instanceType = "t3.xlarge";
config.imageId = "ami-0c55b159cbfafe1f0";
config.keyName = "my-key";
awsProvider.createInstance(config);

// Upload file to S3
awsProvider.uploadFile("my-bucket", "file.txt", "/path/to/local/file.txt");

// Publish metric
awsProvider.publishMetric("ActiveConnections", 42.0f);
```

### Using Resource Manager

```cpp
#include "resource_manager.h"

// Initialize resource manager with AWS
CloudResourceManager manager;
manager.init(CloudProvider::AWS);

// Create streaming server
std::string serverId = manager.createStreamingServer(100);

// Setup auto-scaling
manager.setupAutoScaling(serverId, 3, 10);

// Estimate costs
float monthlyCost = manager.estimateMonthlyCost();
std::cout << "Estimated monthly cost: $" << monthlyCost << std::endl;

// Cleanup
manager.cleanup();
```

## Prerequisites

### AWS
- AWS CLI installed and configured
- IAM credentials with appropriate permissions
- EC2, S3, RDS, CloudWatch access

### Azure
- Azure CLI installed and configured
- Azure subscription
- Resource group created
- Appropriate RBAC permissions

### GCP
- gcloud CLI installed and configured
- GCP project created
- Service account with necessary roles
- APIs enabled (Compute Engine, Cloud Storage, Cloud SQL)

## Configuration

Cloud provider credentials should be managed securely:

1. **Environment Variables** (Recommended for AWS)
   ```bash
   export AWS_ACCESS_KEY_ID="your-access-key"
   export AWS_SECRET_ACCESS_KEY="your-secret-key"
   export AWS_DEFAULT_REGION="us-east-1"
   ```

2. **Configuration Files**
   - AWS: `~/.aws/credentials`
   - Azure: `~/.azure/config`
   - GCP: Service account JSON file

3. **Secrets Management** (Production)
   - AWS Secrets Manager
   - Azure Key Vault
   - GCP Secret Manager

## Building

The cloud provider modules are C++ components that can be built with CMake or included in your build system.

```bash
# Using CMake
mkdir build && cd build
cmake ..
make
```

## Security Considerations

1. **Never commit credentials** to source control
2. **Use IAM roles** when possible instead of access keys
3. **Rotate credentials** regularly
4. **Use least-privilege** access policies
5. **Enable encryption** at rest and in transit
6. **Monitor API calls** through cloud provider logging

## Testing

```cpp
// Mock testing
#include "cloud_provider.h"

class MockCloudProvider : public CloudProvider {
    // Implement mock methods for testing
};
```

## Troubleshooting

### Common Issues

1. **Authentication Failures**
   - Verify credentials are set correctly
   - Check IAM permissions
   - Ensure CLI tools are authenticated

2. **Resource Creation Fails**
   - Check quota limits
   - Verify network connectivity
   - Review cloud provider service health

3. **API Rate Limiting**
   - Implement exponential backoff
   - Use batch operations where possible
   - Request quota increases if needed

## Future Enhancements

- [ ] Support for additional cloud providers (DigitalOcean, Linode)
- [ ] Async/non-blocking operations
- [ ] Better error handling and retries
- [ ] Cloud cost analytics dashboard
- [ ] Multi-cloud resource orchestration
- [ ] Terraform state integration

## License

MIT License - See root LICENSE file
