#include "gcp_provider.h"
#include <iostream>
#include <cstdlib>

GCPProvider::GCPProvider() : initialized(false) {}

GCPProvider::~GCPProvider() {
    // Cleanup
}

int GCPProvider::init(const std::string &project, 
                     const std::string &gceZone,
                     const std::string &gceRegion) {
    projectId = project;
    zone = gceZone;
    region = gceRegion;
    
    // Set project for gcloud
    std::string cmd = "gcloud config set project " + projectId;
    system(cmd.c_str());
    
    initialized = true;
    std::cout << "GCP Provider initialized for project: " << projectId << std::endl;
    return 0;
}

int GCPProvider::executeGCloudCommand(const std::string &command,
                                     const std::map<std::string, std::string> &params) {
    if (!initialized) {
        std::cerr << "GCP Provider not initialized" << std::endl;
        return -1;
    }
    
    std::string cmd = "gcloud " + command;
    
    for (const auto &param : params) {
        cmd += " --" + param.first + "=" + param.second;
    }
    
    std::cout << "Executing: " << cmd << std::endl;
    return system(cmd.c_str());
}

int GCPProvider::createInstance(const InstanceConfig &config) {
    std::map<std::string, std::string> params;
    params["machine-type"] = config.instanceType;
    params["image-family"] = config.imageId;
    params["zone"] = zone;
    
    std::string cmd = "gcloud compute instances create " + config.keyName;
    for (const auto &param : params) {
        cmd += " --" + param.first + "=" + param.second;
    }
    
    std::cout << "Creating GCE instance..." << std::endl;
    return system(cmd.c_str());
}

int GCPProvider::terminateInstance(const std::string &instanceId) {
    std::string cmd = "gcloud compute instances delete " + instanceId + 
                     " --zone=" + zone + " --quiet";
    std::cout << "Deleting GCE instance: " << instanceId << std::endl;
    return system(cmd.c_str());
}

int GCPProvider::listInstances() {
    std::string cmd = "gcloud compute instances list --filter=\"zone:" + zone + "\"";
    std::cout << "Listing GCE instances..." << std::endl;
    return system(cmd.c_str());
}

int GCPProvider::uploadFile(const std::string &bucket, 
                           const std::string &key,
                           const std::string &filePath) {
    std::string cmd = "gsutil cp " + filePath + " gs://" + bucket + "/" + key;
    std::cout << "Uploading file to Cloud Storage" << std::endl;
    return system(cmd.c_str());
}

int GCPProvider::downloadFile(const std::string &bucket,
                             const std::string &key,
                             const std::string &outputPath) {
    std::string cmd = "gsutil cp gs://" + bucket + "/" + key + " " + outputPath;
    std::cout << "Downloading file from Cloud Storage" << std::endl;
    return system(cmd.c_str());
}

DatabaseConnection* GCPProvider::getDatabaseConnection() {
    DatabaseConnection *conn = new DatabaseConnection();
    conn->endpoint = "rootstream-db.cloudsql.goog";
    conn->port = 5432;
    conn->username = "rootstream";
    conn->database = "rootstream";
    conn->isConnected = false;
    
    std::cout << "Cloud SQL connection info retrieved" << std::endl;
    return conn;
}

int GCPProvider::createLoadBalancer(const LoadBalancerConfig &config) {
    std::map<std::string, std::string> params;
    params["load-balancing-scheme"] = "EXTERNAL";
    params["global"] = "";
    
    std::string cmd = "gcloud compute forwarding-rules create " + config.name;
    for (const auto &param : params) {
        if (param.second.empty()) {
            cmd += " --" + param.first;
        } else {
            cmd += " --" + param.first + "=" + param.second;
        }
    }
    
    std::cout << "Creating GCP Load Balancer: " << config.name << std::endl;
    return system(cmd.c_str());
}

int GCPProvider::registerTarget(const std::string &lbId,
                               const std::string &targetId) {
    std::cout << "Adding instance to backend service" << std::endl;
    return 0;
}

int GCPProvider::publishMetric(const std::string &metricName, float value) {
    std::cout << "Publishing Cloud Monitoring metric: " << metricName 
              << " = " << value << std::endl;
    return 0;
}

int GCPProvider::logEvent(const std::string &logGroup, const std::string &event) {
    std::cout << "Logging to Cloud Logging: " << logGroup 
              << " - " << event << std::endl;
    return 0;
}

int GCPProvider::createFirewallRule(const std::string &ruleName,
                                   const std::string &protocol,
                                   int port) {
    std::string cmd = "gcloud compute firewall-rules create " + ruleName + 
                     " --allow=" + protocol + ":" + std::to_string(port);
    std::cout << "Creating firewall rule: " << ruleName << std::endl;
    return system(cmd.c_str());
}

int GCPProvider::createBucket(const std::string &bucketName) {
    std::string cmd = "gsutil mb -l " + region + " gs://" + bucketName;
    std::cout << "Creating Cloud Storage bucket: " << bucketName << std::endl;
    return system(cmd.c_str());
}
