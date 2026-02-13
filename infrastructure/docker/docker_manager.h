#ifndef DOCKER_MANAGER_H
#define DOCKER_MANAGER_H

#include <string>
#include <map>
#include <vector>

struct DockerContainerConfig {
    std::string name;
    std::string image;
    std::map<std::string, std::string> env;
    std::vector<std::string> ports;  // Format: "host:container"
    std::vector<std::string> volumes; // Format: "host:container"
    bool detached;
    std::string network;
};

/**
 * DockerManager - Docker container and image management
 * Handles building, pushing, pulling images and running containers
 */
class DockerManager {
private:
    bool initialized;
    std::string registryUrl;
    
    int executeDockerCommand(const std::string &command);
    
public:
    DockerManager();
    ~DockerManager();
    
    int init();
    int setRegistry(const std::string &registry);
    
    // Image management
    int buildImage(const std::string &dockerfilePath,
                  const std::string &imageName,
                  const std::string &tag);
    
    int pushImage(const std::string &imageName,
                 const std::string &tag);
    
    int pullImage(const std::string &imageName,
                 const std::string &tag);
    
    int tagImage(const std::string &sourceImage,
                const std::string &targetImage);
    
    int listImages();
    int removeImage(const std::string &imageName);
    
    // Container management
    int runContainer(const DockerContainerConfig &config);
    int stopContainer(const std::string &containerId);
    int removeContainer(const std::string &containerId);
    int listContainers(bool all = false);
    int getContainerLogs(const std::string &containerId);
    
    // Docker Compose
    int composeUp(const std::string &composeFilePath);
    int composeDown(const std::string &composeFilePath);
    int composePs(const std::string &composeFilePath);
    
    // Network management
    int createNetwork(const std::string &networkName);
    int removeNetwork(const std::string &networkName);
    
    void cleanup();
};

#endif // DOCKER_MANAGER_H
