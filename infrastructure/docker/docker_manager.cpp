#include "docker_manager.h"
#include <iostream>
#include <sstream>
#include <cstdlib>

DockerManager::DockerManager() : initialized(false) {}

DockerManager::~DockerManager() {
    cleanup();
}

int DockerManager::init() {
    // Check if Docker is available
    int result = system("docker --version > /dev/null 2>&1");
    if (result != 0) {
        std::cerr << "Docker is not installed or not accessible" << std::endl;
        return -1;
    }
    
    initialized = true;
    std::cout << "DockerManager initialized" << std::endl;
    return 0;
}

int DockerManager::setRegistry(const std::string &registry) {
    registryUrl = registry;
    std::cout << "Registry set to: " << registryUrl << std::endl;
    return 0;
}

int DockerManager::executeDockerCommand(const std::string &command) {
    if (!initialized) {
        std::cerr << "DockerManager not initialized" << std::endl;
        return -1;
    }
    
    std::string cmd = "docker " + command;
    std::cout << "Executing: " << cmd << std::endl;
    return system(cmd.c_str());
}

int DockerManager::buildImage(const std::string &dockerfilePath,
                              const std::string &imageName,
                              const std::string &tag) {
    std::cout << "Building Docker image: " << imageName << ":" << tag << std::endl;
    
    std::string cmd = "build -t " + imageName + ":" + tag + " -f " + dockerfilePath + " .";
    return executeDockerCommand(cmd);
}

int DockerManager::pushImage(const std::string &imageName,
                            const std::string &tag) {
    std::string fullImage = imageName + ":" + tag;
    if (!registryUrl.empty()) {
        fullImage = registryUrl + "/" + fullImage;
    }
    
    std::cout << "Pushing image: " << fullImage << std::endl;
    return executeDockerCommand("push " + fullImage);
}

int DockerManager::pullImage(const std::string &imageName,
                            const std::string &tag) {
    std::string fullImage = imageName + ":" + tag;
    if (!registryUrl.empty()) {
        fullImage = registryUrl + "/" + fullImage;
    }
    
    std::cout << "Pulling image: " << fullImage << std::endl;
    return executeDockerCommand("pull " + fullImage);
}

int DockerManager::tagImage(const std::string &sourceImage,
                           const std::string &targetImage) {
    std::cout << "Tagging image: " << sourceImage << " -> " << targetImage << std::endl;
    return executeDockerCommand("tag " + sourceImage + " " + targetImage);
}

int DockerManager::listImages() {
    return executeDockerCommand("images");
}

int DockerManager::removeImage(const std::string &imageName) {
    std::cout << "Removing image: " << imageName << std::endl;
    return executeDockerCommand("rmi " + imageName);
}

int DockerManager::runContainer(const DockerContainerConfig &config) {
    std::cout << "Running container: " << config.name << std::endl;
    
    std::stringstream cmd;
    cmd << "run";
    
    if (config.detached) {
        cmd << " -d";
    }
    
    cmd << " --name " << config.name;
    
    // Add environment variables
    for (const auto &env : config.env) {
        cmd << " -e " << env.first << "=" << env.second;
    }
    
    // Add port mappings
    for (const auto &port : config.ports) {
        cmd << " -p " << port;
    }
    
    // Add volume mounts
    for (const auto &volume : config.volumes) {
        cmd << " -v " << volume;
    }
    
    // Add network
    if (!config.network.empty()) {
        cmd << " --network " << config.network;
    }
    
    cmd << " " << config.image;
    
    return executeDockerCommand(cmd.str());
}

int DockerManager::stopContainer(const std::string &containerId) {
    std::cout << "Stopping container: " << containerId << std::endl;
    return executeDockerCommand("stop " + containerId);
}

int DockerManager::removeContainer(const std::string &containerId) {
    std::cout << "Removing container: " << containerId << std::endl;
    return executeDockerCommand("rm " + containerId);
}

int DockerManager::listContainers(bool all) {
    std::string cmd = "ps";
    if (all) {
        cmd += " -a";
    }
    return executeDockerCommand(cmd);
}

int DockerManager::getContainerLogs(const std::string &containerId) {
    return executeDockerCommand("logs " + containerId);
}

int DockerManager::composeUp(const std::string &composeFilePath) {
    std::cout << "Starting Docker Compose services" << std::endl;
    std::string cmd = "docker-compose -f " + composeFilePath + " up -d";
    std::cout << "Executing: " << cmd << std::endl;
    return system(cmd.c_str());
}

int DockerManager::composeDown(const std::string &composeFilePath) {
    std::cout << "Stopping Docker Compose services" << std::endl;
    std::string cmd = "docker-compose -f " + composeFilePath + " down";
    std::cout << "Executing: " << cmd << std::endl;
    return system(cmd.c_str());
}

int DockerManager::composePs(const std::string &composeFilePath) {
    std::string cmd = "docker-compose -f " + composeFilePath + " ps";
    std::cout << "Executing: " << cmd << std::endl;
    return system(cmd.c_str());
}

int DockerManager::createNetwork(const std::string &networkName) {
    std::cout << "Creating Docker network: " << networkName << std::endl;
    return executeDockerCommand("network create " + networkName);
}

int DockerManager::removeNetwork(const std::string &networkName) {
    std::cout << "Removing Docker network: " << networkName << std::endl;
    return executeDockerCommand("network rm " + networkName);
}

void DockerManager::cleanup() {
    std::cout << "Cleaning up DockerManager..." << std::endl;
}
