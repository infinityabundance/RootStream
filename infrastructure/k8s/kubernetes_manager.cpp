#include "kubernetes_manager.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <fstream>

KubernetesManager::KubernetesManager() 
    : current_namespace("default"), initialized(false) {}

KubernetesManager::~KubernetesManager() {
    cleanup();
}

int KubernetesManager::init(const std::string &kubeconfig) {
    kubeconfig_path = kubeconfig;
    
    if (!kubeconfig_path.empty()) {
        setenv("KUBECONFIG", kubeconfig_path.c_str(), 1);
    }
    
    // Test kubectl connection
    int result = system("kubectl cluster-info > /dev/null 2>&1");
    if (result != 0) {
        std::cerr << "Failed to connect to Kubernetes cluster" << std::endl;
        return -1;
    }
    
    initialized = true;
    std::cout << "KubernetesManager initialized" << std::endl;
    return 0;
}

int KubernetesManager::executeKubectl(const std::string &command) {
    if (!initialized) {
        std::cerr << "KubernetesManager not initialized" << std::endl;
        return -1;
    }
    
    std::string cmd = "kubectl ";
    if (!current_namespace.empty()) {
        cmd += "-n " + current_namespace + " ";
    }
    cmd += command;
    
    std::cout << "Executing: " << cmd << std::endl;
    return system(cmd.c_str());
}

int KubernetesManager::createDeployment(const K8sDeploymentSpec &spec) {
    std::cout << "Creating deployment: " << spec.name << std::endl;
    
    // Generate deployment YAML
    std::stringstream yaml;
    yaml << "apiVersion: apps/v1\n";
    yaml << "kind: Deployment\n";
    yaml << "metadata:\n";
    yaml << "  name: " << spec.name << "\n";
    yaml << "spec:\n";
    yaml << "  replicas: " << spec.replicas << "\n";
    yaml << "  selector:\n";
    yaml << "    matchLabels:\n";
    for (const auto &label : spec.labels) {
        yaml << "      " << label.first << ": " << label.second << "\n";
    }
    yaml << "  template:\n";
    yaml << "    metadata:\n";
    yaml << "      labels:\n";
    for (const auto &label : spec.labels) {
        yaml << "        " << label.first << ": " << label.second << "\n";
    }
    yaml << "    spec:\n";
    yaml << "      containers:\n";
    yaml << "      - name: " << spec.name << "\n";
    yaml << "        image: " << spec.image << "\n";
    yaml << "        ports:\n";
    yaml << "        - containerPort: " << spec.containerPort << "\n";
    
    if (!spec.env.empty()) {
        yaml << "        env:\n";
        for (const auto &envVar : spec.env) {
            yaml << "        - name: " << envVar.first << "\n";
            yaml << "          value: \"" << envVar.second << "\"\n";
        }
    }
    
    yaml << "        resources:\n";
    yaml << "          requests:\n";
    yaml << "            cpu: " << spec.cpuRequest << "\n";
    yaml << "            memory: " << spec.memoryRequest << "\n";
    yaml << "          limits:\n";
    yaml << "            cpu: " << spec.cpuLimit << "\n";
    yaml << "            memory: " << spec.memoryLimit << "\n";
    
    // Write to temp file and apply
    std::string tempFile = "/tmp/deployment-" + spec.name + ".yaml";
    std::ofstream out(tempFile);
    out << yaml.str();
    out.close();
    
    return executeKubectl("apply -f " + tempFile);
}

int KubernetesManager::updateDeployment(const std::string &deploymentName,
                                       const K8sDeploymentSpec &spec) {
    std::cout << "Updating deployment: " << deploymentName << std::endl;
    return createDeployment(spec);  // kubectl apply is idempotent
}

int KubernetesManager::deleteDeployment(const std::string &deploymentName) {
    std::cout << "Deleting deployment: " << deploymentName << std::endl;
    return executeKubectl("delete deployment " + deploymentName);
}

int KubernetesManager::createService(const K8sServiceSpec &spec) {
    std::cout << "Creating service: " << spec.name << std::endl;
    
    std::stringstream yaml;
    yaml << "apiVersion: v1\n";
    yaml << "kind: Service\n";
    yaml << "metadata:\n";
    yaml << "  name: " << spec.name << "\n";
    yaml << "spec:\n";
    yaml << "  type: " << spec.type << "\n";
    yaml << "  ports:\n";
    yaml << "  - port: " << spec.port << "\n";
    yaml << "    targetPort: " << spec.targetPort << "\n";
    yaml << "  selector:\n";
    for (const auto &selector : spec.selector) {
        yaml << "    " << selector.first << ": " << selector.second << "\n";
    }
    
    std::string tempFile = "/tmp/service-" + spec.name + ".yaml";
    std::ofstream out(tempFile);
    out << yaml.str();
    out.close();
    
    return executeKubectl("apply -f " + tempFile);
}

int KubernetesManager::exposeService(const std::string &serviceName,
                                    uint16_t port, uint16_t targetPort) {
    std::string cmd = "expose deployment " + serviceName + 
                     " --port=" + std::to_string(port) +
                     " --target-port=" + std::to_string(targetPort);
    return executeKubectl(cmd);
}

int KubernetesManager::deleteService(const std::string &serviceName) {
    return executeKubectl("delete service " + serviceName);
}

int KubernetesManager::createStatefulSet(const K8sStatefulSetSpec &spec) {
    std::cout << "Creating StatefulSet: " << spec.name << std::endl;
    
    std::string cmd = "kubectl create statefulset " + spec.name +
                     " --image=" + spec.image +
                     " --replicas=" + std::to_string(spec.replicas);
    
    return system(cmd.c_str());
}

int KubernetesManager::deleteStatefulSet(const std::string &name) {
    return executeKubectl("delete statefulset " + name);
}

int KubernetesManager::createConfigMap(const std::string &name,
                                      const std::map<std::string, std::string> &data) {
    std::cout << "Creating ConfigMap: " << name << std::endl;
    
    std::string cmd = "create configmap " + name;
    for (const auto &entry : data) {
        cmd += " --from-literal=" + entry.first + "=" + entry.second;
    }
    
    return executeKubectl(cmd);
}

int KubernetesManager::createSecret(const std::string &name,
                                   const std::map<std::string, std::string> &data) {
    std::cout << "Creating Secret: " << name << std::endl;
    
    std::string cmd = "create secret generic " + name;
    for (const auto &entry : data) {
        cmd += " --from-literal=" + entry.first + "=" + entry.second;
    }
    
    return executeKubectl(cmd);
}

int KubernetesManager::createHPA(const std::string &deploymentName,
                                uint32_t minReplicas, uint32_t maxReplicas,
                                float cpuThreshold) {
    std::cout << "Creating HorizontalPodAutoscaler for: " << deploymentName << std::endl;
    
    std::string cmd = "autoscale deployment " + deploymentName +
                     " --min=" + std::to_string(minReplicas) +
                     " --max=" + std::to_string(maxReplicas) +
                     " --cpu-percent=" + std::to_string(static_cast<int>(cpuThreshold));
    
    return executeKubectl(cmd);
}

int KubernetesManager::deleteHPA(const std::string &hpaName) {
    return executeKubectl("delete hpa " + hpaName);
}

int KubernetesManager::getDeploymentStatus(const std::string &deploymentName) {
    return executeKubectl("get deployment " + deploymentName);
}

int KubernetesManager::getNodeStatus() {
    return executeKubectl("get nodes");
}

int KubernetesManager::getPodLogs(const std::string &podName) {
    return executeKubectl("logs " + podName);
}

int KubernetesManager::setNamespace(const std::string &ns) {
    current_namespace = ns;
    std::cout << "Namespace set to: " << current_namespace << std::endl;
    return 0;
}

void KubernetesManager::cleanup() {
    std::cout << "Cleaning up KubernetesManager..." << std::endl;
}
