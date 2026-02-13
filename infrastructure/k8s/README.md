# Kubernetes Management

This module provides Kubernetes orchestration capabilities for RootStream, enabling automated deployment, scaling, and management of containerized applications.

## Overview

The Kubernetes Manager provides a C++ interface to Kubernetes clusters, allowing RootStream to:

- Deploy and manage applications
- Create and configure services
- Manage StatefulSets for databases
- Configure auto-scaling (HPA)
- Handle ConfigMaps and Secrets
- Monitor deployment health

## Features

### Deployment Management
- Create, update, and delete deployments
- Configure replica counts
- Set resource limits and requests
- Health checks (liveness and readiness probes)

### Service Management
- Create and expose services
- LoadBalancer, NodePort, and ClusterIP support
- Service discovery

### Auto-Scaling
- Horizontal Pod Autoscaler (HPA) configuration
- CPU and memory-based scaling
- Custom metrics support

### StatefulSets
- Manage stateful applications (databases)
- Persistent volume claims
- Ordered deployment and scaling

### Configuration Management
- ConfigMaps for application configuration
- Secrets for sensitive data
- Environment variable injection

## Usage

### Initialize Manager

```cpp
#include "kubernetes_manager.h"

KubernetesManager k8s;
k8s.init("/path/to/kubeconfig");
k8s.setNamespace("rootstream");
```

### Create a Deployment

```cpp
K8sDeploymentSpec spec;
spec.name = "rootstream-server";
spec.image = "rootstream/server:latest";
spec.replicas = 3;
spec.containerPort = 5000;
spec.cpuRequest = "500m";
spec.memoryRequest = "512Mi";
spec.cpuLimit = "2000m";
spec.memoryLimit = "2Gi";

spec.labels["app"] = "rootstream";
spec.labels["tier"] = "backend";

spec.env["LOG_LEVEL"] = "info";
spec.env["DATABASE_URL"] = "postgresql://...";

k8s.createDeployment(spec);
```

### Create a Service

```cpp
K8sServiceSpec serviceSpec;
serviceSpec.name = "rootstream-service";
serviceSpec.type = "LoadBalancer";
serviceSpec.port = 80;
serviceSpec.targetPort = 5000;
serviceSpec.selector["app"] = "rootstream";

k8s.createService(serviceSpec);
```

### Configure Auto-Scaling

```cpp
// Scale between 3 and 10 replicas based on 70% CPU utilization
k8s.createHPA("rootstream-server", 3, 10, 70.0f);
```

### Manage ConfigMaps

```cpp
std::map<std::string, std::string> configData;
configData["app.conf"] = "setting1=value1\nsetting2=value2";
configData["redis-url"] = "redis://redis:6379";

k8s.createConfigMap("rootstream-config", configData);
```

### Manage Secrets

```cpp
std::map<std::string, std::string> secretData;
secretData["database-password"] = "supersecret";
secretData["api-key"] = "api-key-value";

k8s.createSecret("rootstream-secrets", secretData);
```

## Prerequisites

1. **kubectl** installed and configured
2. **kubeconfig** file with cluster access
3. **Kubernetes cluster** running (EKS, GKE, AKS, or local)
4. **Namespace** created (or use default)

## Setup

### 1. Configure kubectl

```bash
# AWS EKS
aws eks update-kubeconfig --region us-east-1 --name rootstream-cluster

# GCP GKE
gcloud container clusters get-credentials rootstream-cluster --zone us-central1-a

# Azure AKS
az aks get-credentials --resource-group rootstream-rg --name rootstream-cluster

# Local (minikube)
minikube start
```

### 2. Create Namespace

```bash
kubectl create namespace rootstream
```

### 3. Verify Connection

```bash
kubectl cluster-info
kubectl get nodes
```

## Building

```bash
# Compile with C++17
g++ -std=c++17 kubernetes_manager.cpp -o k8s_manager

# Or include in CMakeLists.txt
add_executable(k8s_manager kubernetes_manager.cpp)
target_compile_features(k8s_manager PRIVATE cxx_std_17)
```

## Kubernetes Resources

### Deployment Example YAML

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: rootstream-server
spec:
  replicas: 3
  selector:
    matchLabels:
      app: rootstream
  template:
    metadata:
      labels:
        app: rootstream
    spec:
      containers:
      - name: rootstream-server
        image: rootstream/server:latest
        ports:
        - containerPort: 5000
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: 2000m
            memory: 2Gi
```

### Service Example YAML

```yaml
apiVersion: v1
kind: Service
metadata:
  name: rootstream-service
spec:
  type: LoadBalancer
  ports:
  - port: 80
    targetPort: 5000
  selector:
    app: rootstream
```

## Monitoring

### Get Deployment Status

```cpp
k8s.getDeploymentStatus("rootstream-server");
```

### Get Node Status

```cpp
k8s.getNodeStatus();
```

### Get Pod Logs

```cpp
k8s.getPodLogs("rootstream-server-abc123-xyz");
```

### Using kubectl

```bash
# Watch pods
kubectl get pods -n rootstream -w

# View logs
kubectl logs -n rootstream -l app=rootstream --tail=100

# Describe deployment
kubectl describe deployment rootstream-server -n rootstream

# Check HPA
kubectl get hpa -n rootstream
```

## Best Practices

1. **Resource Limits**: Always set CPU and memory limits
2. **Health Checks**: Configure liveness and readiness probes
3. **Rolling Updates**: Use rolling update strategy for zero-downtime deployments
4. **Secrets**: Never commit secrets to Git; use Kubernetes Secrets
5. **Namespaces**: Use separate namespaces for different environments
6. **Labels**: Use consistent labeling for resource organization
7. **Monitoring**: Enable metrics-server for HPA functionality

## Troubleshooting

### Pod Not Starting

```bash
kubectl describe pod <pod-name> -n rootstream
kubectl logs <pod-name> -n rootstream
```

### Service Not Accessible

```bash
kubectl get svc -n rootstream
kubectl get endpoints -n rootstream
```

### HPA Not Scaling

```bash
kubectl get hpa -n rootstream
kubectl top pods -n rootstream
kubectl top nodes
```

## Advanced Usage

### StatefulSet for Database

```cpp
K8sStatefulSetSpec statefulSpec;
statefulSpec.name = "postgres";
statefulSpec.serviceName = "postgres";
statefulSpec.replicas = 3;
statefulSpec.image = "postgres:15";

k8s.createStatefulSet(statefulSpec);
```

### Custom Metrics

For custom metrics-based auto-scaling, integrate with Prometheus and configure custom HPA metrics.

## Integration with Helm

For easier management, consider using Helm charts (see `../helm/` directory).

## License

MIT License - See root LICENSE file
