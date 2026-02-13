#ifndef KUBERNETES_MANAGER_H
#define KUBERNETES_MANAGER_H

#include <string>
#include <map>
#include <vector>

// K8s specifications
struct K8sDeploymentSpec {
    std::string name;
    std::string image;
    int replicas;
    std::map<std::string, std::string> labels;
    std::map<std::string, std::string> env;
    int containerPort;
    std::string cpuRequest;
    std::string memoryRequest;
    std::string cpuLimit;
    std::string memoryLimit;
};

struct K8sServiceSpec {
    std::string name;
    std::string type;  // ClusterIP, NodePort, LoadBalancer
    int port;
    int targetPort;
    std::map<std::string, std::string> selector;
};

struct K8sStatefulSetSpec {
    std::string name;
    std::string serviceName;
    int replicas;
    std::string image;
    std::vector<std::string> volumeClaimTemplates;
};

/**
 * KubernetesManager - Kubernetes cluster management and orchestration
 * Handles deployments, services, StatefulSets, and auto-scaling
 */
class KubernetesManager {
private:
    std::string kubeconfig_path;
    std::string current_namespace;
    bool initialized;
    
    int executeKubectl(const std::string &command);
    
public:
    KubernetesManager();
    ~KubernetesManager();
    
    int init(const std::string &kubeconfig);
    
    // Deployment management
    int createDeployment(const K8sDeploymentSpec &spec);
    int updateDeployment(const std::string &deploymentName,
                        const K8sDeploymentSpec &spec);
    int deleteDeployment(const std::string &deploymentName);
    
    // Service management
    int createService(const K8sServiceSpec &spec);
    int exposeService(const std::string &serviceName,
                     uint16_t port, uint16_t targetPort);
    int deleteService(const std::string &serviceName);
    
    // StatefulSet for databases
    int createStatefulSet(const K8sStatefulSetSpec &spec);
    int deleteStatefulSet(const std::string &name);
    
    // ConfigMap and Secrets
    int createConfigMap(const std::string &name,
                       const std::map<std::string, std::string> &data);
    int createSecret(const std::string &name,
                    const std::map<std::string, std::string> &data);
    
    // Auto-scaling
    int createHPA(const std::string &deploymentName,
                 uint32_t minReplicas, uint32_t maxReplicas,
                 float cpuThreshold);
    int deleteHPA(const std::string &hpaName);
    
    // Monitoring
    int getDeploymentStatus(const std::string &deploymentName);
    int getNodeStatus();
    int getPodLogs(const std::string &podName);
    
    // Namespace management
    int setNamespace(const std::string &ns);
    std::string getCurrentNamespace() const { return current_namespace; }
    
    void cleanup();
};

#endif // KUBERNETES_MANAGER_H
