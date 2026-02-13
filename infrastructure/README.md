# RootStream Cloud Infrastructure

**Phase 24.1: Cloud Architecture & Infrastructure Setup**

This directory contains all infrastructure components for deploying and managing RootStream in cloud environments.

## 🎯 Overview

RootStream's cloud infrastructure provides:
- Multi-cloud provider support (AWS, Azure, GCP)
- Kubernetes orchestration
- Containerization with Docker
- Infrastructure as Code with Terraform
- Automated deployment and scaling
- Comprehensive monitoring and health checks

## 📁 Directory Structure

```
infrastructure/
├── cloud/              # Cloud provider abstraction layer
│   ├── cloud_provider.h
│   ├── aws_provider.h/cpp
│   ├── azure_provider.h/cpp
│   ├── gcp_provider.h/cpp
│   ├── resource_manager.h/cpp
│   └── README.md
├── k8s/                # Kubernetes management
│   ├── kubernetes_manager.h/cpp
│   └── README.md
├── docker/             # Docker containers
│   ├── docker_manager.h/cpp
│   ├── rootstream-server.Dockerfile
│   ├── rootstream-client.Dockerfile
│   ├── docker-compose.yml
│   └── README.md
├── helm/               # Helm charts for Kubernetes
│   └── rootstream/
│       ├── Chart.yaml
│       ├── values.yaml
│       └── templates/
│           ├── deployment.yaml
│           ├── service.yaml
│           ├── ingress.yaml
│           ├── hpa.yaml
│           └── ...
├── terraform/          # Infrastructure as Code
│   ├── main.tf
│   ├── variables.tf
│   ├── outputs.tf
│   └── README.md
├── monitoring/         # Health checks and monitoring
│   ├── health_check.h/cpp
│   └── README.md
├── scripts/            # Deployment automation
│   ├── deploy.sh
│   ├── scale.sh
│   └── backup.sh
└── README.md          # This file
```

## 🚀 Quick Start

### Prerequisites

1. **Tools**
   ```bash
   # Install required tools
   brew install terraform kubectl helm docker aws-cli
   
   # Or on Linux
   apt-get install terraform kubectl helm docker.io awscli
   ```

2. **Cloud Provider Credentials**
   - AWS: Configure with `aws configure`
   - Azure: Configure with `az login`
   - GCP: Configure with `gcloud auth login`

3. **Docker**
   - Ensure Docker daemon is running
   - Login to container registry if needed

### Deployment Options

#### Option 1: Quick Docker Compose (Development)

```bash
cd docker
docker-compose up -d
```

#### Option 2: Kubernetes with Helm (Production)

```bash
# 1. Deploy infrastructure with Terraform
cd terraform
terraform init
terraform plan -out=tfplan
terraform apply tfplan

# 2. Configure kubectl
aws eks update-kubeconfig --region us-east-1 --name rootstream-cluster

# 3. Deploy with Helm
cd ../helm
helm install rootstream ./rootstream -n rootstream --create-namespace

# 4. Verify deployment
kubectl get all -n rootstream
```

#### Option 3: Automated Deployment Script

```bash
cd scripts
./deploy.sh
```

## 🏗️ Architecture

### Cloud Infrastructure

```
┌─────────────────────────────────────────────────────┐
│              Cloud Provider Layer                    │
│   (AWS / Azure / GCP abstraction)                   │
└────────────────┬────────────────────────────────────┘
                 │
         ┌───────┴────────┐
         │                │
┌────────▼────────┐  ┌────▼────────────┐  ┌─────────────┐
│  Compute        │  │  Storage        │  │  Databases  │
│  (EKS/GKE/AKS) │  │  (S3/Blob/GCS) │  │  (RDS/SQL)  │
└────────┬────────┘  └─────────────────┘  └─────────────┘
         │
┌────────▼──────────────────────────────────────────────┐
│          Kubernetes Cluster                            │
│                                                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐│
│  │   Ingress    │  │  RootStream  │  │  Monitoring  ││
│  │ Controller   │  │  Deployment  │  │   & Logs     ││
│  └──────────────┘  └──────────────┘  └──────────────┘│
│                                                        │
│  ┌──────────────┐  ┌──────────────┐                  │
│  │  PostgreSQL  │  │    Redis     │                  │
│  │ StatefulSet  │  │    Cache     │                  │
│  └──────────────┘  └──────────────┘                  │
└────────────────────────────────────────────────────────┘
```

### Multi-Cloud Support

The cloud provider abstraction allows seamless deployment across:

| Provider | Compute | Storage | Database | Monitoring |
|----------|---------|---------|----------|------------|
| **AWS** | EKS, EC2 | S3 | RDS | CloudWatch |
| **Azure** | AKS, VMs | Blob | SQL DB | App Insights |
| **GCP** | GKE, GCE | Cloud Storage | Cloud SQL | Cloud Monitoring |

## 📦 Components

### 1. Cloud Provider Abstraction (`cloud/`)

Unified interface for cloud operations across AWS, Azure, and GCP.

**Key Features**:
- VM instance management
- Storage operations (upload/download)
- Database connections
- Load balancer configuration
- Metrics and logging

**Usage**:
```cpp
#include "aws_provider.h"

AWSProvider aws;
aws.init("us-east-1", "key", "secret");
aws.createInstance(config);
```

[📖 Full Documentation](cloud/README.md)

### 2. Kubernetes Management (`k8s/`)

Programmatic Kubernetes cluster management.

**Key Features**:
- Deployment management
- Service creation and exposure
- StatefulSet for databases
- ConfigMap and Secret management
- Horizontal Pod Autoscaling

**Usage**:
```cpp
#include "kubernetes_manager.h"

KubernetesManager k8s;
k8s.init("/path/to/kubeconfig");
k8s.createDeployment(spec);
k8s.createHPA("deployment", 3, 10, 70.0f);
```

[📖 Full Documentation](k8s/README.md)

### 3. Docker Management (`docker/`)

Container image building and management.

**Key Features**:
- Image build, tag, push, pull
- Container lifecycle management
- Docker Compose orchestration
- Network management

**Files**:
- `rootstream-server.Dockerfile`: Production server image
- `rootstream-client.Dockerfile`: Client application image
- `docker-compose.yml`: Multi-container setup

[📖 Full Documentation](docker/README.md)

### 4. Helm Charts (`helm/`)

Kubernetes application packages.

**Key Features**:
- Parameterized deployments
- Version management
- Rollback support
- Template-based configuration

**Usage**:
```bash
helm install rootstream ./helm/rootstream \
  --set image.tag=v1.0.0 \
  --set autoscaling.maxReplicas=20
```

### 5. Terraform IaC (`terraform/`)

Infrastructure as Code for AWS.

**Provisions**:
- VPC with public/private subnets
- EKS cluster with node groups
- RDS PostgreSQL (Multi-AZ)
- ElastiCache Redis cluster
- Application Load Balancer
- S3 storage
- ECR repository

**Usage**:
```bash
cd terraform
terraform init
terraform plan
terraform apply
```

[📖 Full Documentation](terraform/README.md)

### 6. Monitoring (`monitoring/`)

Health checks and metrics collection.

**Key Features**:
- System health monitoring
- Service availability checks
- Resource utilization tracking
- Alert configuration
- Metrics export

**Usage**:
```cpp
#include "health_check.h"

HealthCheckManager health;
health.init();
health.setHealthAlert("cpu", 80.0f);
HealthStatus status = health.getOverallHealth();
```

[📖 Full Documentation](monitoring/README.md)

### 7. Deployment Scripts (`scripts/`)

Automation scripts for common operations.

**Scripts**:
- `deploy.sh`: Full deployment automation
- `scale.sh`: Manual and auto-scaling management
- `backup.sh`: Backup and disaster recovery

**Usage**:
```bash
# Full deployment
./scripts/deploy.sh

# Scale deployment
./scripts/scale.sh

# Create backup
./scripts/backup.sh
```

## 🔧 Configuration

### Environment Variables

```bash
# Cloud Provider
export AWS_REGION=us-east-1
export ENVIRONMENT=production

# Database
export DATABASE_URL=postgresql://user:pass@host:5432/rootstream
export REDIS_URL=redis://redis:6379

# Application
export ROOTSTREAM_MODE=server
export LOG_LEVEL=info
```

### Kubernetes Secrets

```bash
kubectl create secret generic rootstream-db-secret \
  --from-literal=password=supersecret \
  -n rootstream

kubectl create secret generic rootstream-redis-secret \
  --from-literal=password=redispass \
  -n rootstream
```

## 📊 Monitoring & Observability

### Health Endpoints

- `GET /health` - Overall health status
- `GET /ready` - Readiness check
- `GET /metrics` - Prometheus metrics

### Kubernetes Monitoring

```bash
# Watch pods
kubectl get pods -n rootstream -w

# View logs
kubectl logs -n rootstream -l app=rootstream --tail=100 -f

# Check resource usage
kubectl top pods -n rootstream
kubectl top nodes
```

### CloudWatch/Prometheus

Metrics are exported to:
- AWS CloudWatch (for AWS deployments)
- Prometheus (via /metrics endpoint)
- Application Insights (for Azure)
- Cloud Monitoring (for GCP)

## 🔐 Security

### Best Practices

1. **Secrets Management**
   - Never commit secrets to Git
   - Use AWS Secrets Manager / Azure Key Vault / GCP Secret Manager
   - Rotate credentials regularly

2. **Network Security**
   - Private subnets for databases
   - Security groups with minimal access
   - TLS/SSL everywhere

3. **Access Control**
   - RBAC for Kubernetes
   - IAM roles for AWS resources
   - Least privilege principle

4. **Monitoring**
   - Enable CloudTrail/Activity Log
   - Set up security alerts
   - Regular security audits

## 📈 Scaling

### Horizontal Pod Autoscaling

```bash
# Via script
./scripts/scale.sh

# Via kubectl
kubectl autoscale deployment rootstream \
  --min=3 --max=10 --cpu-percent=70 \
  -n rootstream

# Via Helm
helm upgrade rootstream ./helm/rootstream \
  --set autoscaling.enabled=true \
  --set autoscaling.maxReplicas=20
```

### Cluster Autoscaling

Configure in Terraform or cloud provider console.

## 🔄 CI/CD Integration

### GitHub Actions Example

```yaml
name: Deploy to Production

on:
  push:
    branches: [main]

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Build Docker image
        run: docker build -t rootstream:${{ github.sha }} .
      
      - name: Push to registry
        run: docker push rootstream:${{ github.sha }}
      
      - name: Deploy to Kubernetes
        run: |
          kubectl set image deployment/rootstream \
            rootstream=rootstream:${{ github.sha }} \
            -n rootstream
```

## 🧪 Testing

### Local Testing

```bash
# Start with Docker Compose
docker-compose -f docker/docker-compose.yml up

# Test health endpoint
curl http://localhost:5001/health
```

### Load Testing

```bash
# Using Apache Bench
ab -n 10000 -c 100 http://loadbalancer-url/

# Using k6
k6 run loadtest.js
```

## 📚 Documentation

- [Cloud Provider README](cloud/README.md)
- [Kubernetes README](k8s/README.md)
- [Docker README](docker/README.md)
- [Terraform README](terraform/README.md)
- [Monitoring README](monitoring/README.md)

## 🆘 Troubleshooting

### Common Issues

**1. Pods not starting**
```bash
kubectl describe pod <pod-name> -n rootstream
kubectl logs <pod-name> -n rootstream
```

**2. Database connection failed**
- Check security groups
- Verify credentials
- Test connectivity from pod

**3. High memory usage**
- Check resource limits
- Review memory leaks
- Scale vertically or horizontally

**4. Terraform errors**
- Check AWS credentials
- Verify state lock
- Review IAM permissions

## 💰 Cost Optimization

1. **Use auto-scaling** - Scale down during off-peak hours
2. **Reserved instances** - For predictable workloads
3. **Spot instances** - For non-critical workloads
4. **Right-size resources** - Monitor and adjust
5. **Lifecycle policies** - Archive old data to cheaper storage

## 🤝 Contributing

1. Make changes in a feature branch
2. Test locally with Docker Compose
3. Run Terraform plan (don't apply)
4. Submit pull request
5. Wait for CI/CD validation

## 📄 License

MIT License - See root LICENSE file

## ✅ Success Criteria

- [x] Multi-cloud provider abstraction layer
- [x] Kubernetes cluster deployment automation
- [x] Docker containerization
- [x] Infrastructure as Code with Terraform
- [x] Helm charts for application deployment
- [x] Auto-scaling configured
- [x] Monitoring and health checks
- [x] Cost tracking and optimization capabilities
- [x] High availability setup (3+ zones)
- [x] Backup and disaster recovery scripts

---

**Note**: This infrastructure is designed for production use but requires proper configuration, security hardening, and testing before deploying to production environments.
