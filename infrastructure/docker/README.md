# Docker Container Management

This module provides Docker container and image management capabilities for RootStream.

## Overview

The Docker Manager enables:
- Building and managing Docker images
- Running and managing containers
- Docker Compose orchestration
- Container registry operations
- Network management

## Components

### DockerManager Class

C++ interface for Docker operations.

**Files**: `docker_manager.h`, `docker_manager.cpp`

### Dockerfiles

- **rootstream-server.Dockerfile**: Server container image
- **rootstream-client.Dockerfile**: Client container image

### Docker Compose

- **docker-compose.yml**: Multi-container orchestration

## Quick Start

### Build Server Image

```bash
docker build -t rootstream-server:latest -f rootstream-server.Dockerfile ../..
```

### Run with Docker Compose

```bash
docker-compose up -d
```

### View Logs

```bash
docker-compose logs -f rootstream-server
```

## Dockerfile Structure

### Server Dockerfile

```dockerfile
FROM ubuntu:22.04
# Install dependencies
# Copy application
# Configure runtime
# Set entrypoint
```

**Features**:
- Minimal base image (Ubuntu 22.04)
- Runtime dependencies only
- Non-root user
- Health checks
- Exposed ports: 5000/udp, 5001/tcp

### Client Dockerfile

Similar structure optimized for client-side needs.

## Docker Compose Setup

The `docker-compose.yml` includes:

1. **rootstream-server**: Main application server
2. **postgres**: PostgreSQL database
3. **redis**: Redis cache
4. **nginx**: Reverse proxy

### Services

```yaml
services:
  rootstream-server:
    # Application server
  postgres:
    # Database
  redis:
    # Cache
  nginx:
    # Load balancer
```

## Usage with DockerManager

### Initialize

```cpp
#include "docker_manager.h"

DockerManager docker;
docker.init();
docker.setRegistry("myregistry.io");
```

### Build Image

```cpp
docker.buildImage(
    "infrastructure/docker/rootstream-server.Dockerfile",
    "rootstream-server",
    "v1.0.0"
);
```

### Run Container

```cpp
DockerContainerConfig config;
config.name = "rootstream-server-1";
config.image = "rootstream-server:latest";
config.detached = true;

config.env["LOG_LEVEL"] = "info";
config.env["DATABASE_URL"] = "postgresql://...";

config.ports.push_back("5000:5000");
config.ports.push_back("5001:5001");

config.volumes.push_back("/data:/app/data");

docker.runContainer(config);
```

### Push to Registry

```cpp
docker.pushImage("rootstream-server", "latest");
```

### Docker Compose Operations

```cpp
docker.composeUp("docker-compose.yml");
docker.composePs("docker-compose.yml");
docker.composeDown("docker-compose.yml");
```

## Building Images

### Manual Build

```bash
# Server
docker build -t rootstream/server:latest \
  -f infrastructure/docker/rootstream-server.Dockerfile .

# Client
docker build -t rootstream/client:latest \
  -f infrastructure/docker/rootstream-client.Dockerfile .
```

### Multi-platform Build

```bash
docker buildx build --platform linux/amd64,linux/arm64 \
  -t rootstream/server:latest \
  -f infrastructure/docker/rootstream-server.Dockerfile .
```

## Registry Operations

### Tag Image

```bash
docker tag rootstream/server:latest myregistry.io/rootstream/server:latest
```

### Push to Registry

```bash
docker push myregistry.io/rootstream/server:latest
```

### Pull from Registry

```bash
docker pull myregistry.io/rootstream/server:latest
```

## Container Management

### List Containers

```bash
docker ps
docker ps -a  # Including stopped
```

### Stop Container

```bash
docker stop rootstream-server-1
```

### Remove Container

```bash
docker rm rootstream-server-1
```

### View Logs

```bash
docker logs -f rootstream-server-1
```

### Execute Command

```bash
docker exec -it rootstream-server-1 /bin/bash
```

## Docker Compose Commands

### Start Services

```bash
docker-compose up -d
```

### Stop Services

```bash
docker-compose down
```

### View Status

```bash
docker-compose ps
```

### Scale Services

```bash
docker-compose up -d --scale rootstream-server=3
```

### View Logs

```bash
docker-compose logs -f
docker-compose logs -f rootstream-server
```

## Environment Variables

Configure via `.env` file:

```env
ROOTSTREAM_MODE=server
LOG_LEVEL=info
DATABASE_URL=postgresql://user:pass@postgres:5432/rootstream
REDIS_URL=redis://redis:6379
```

## Volumes

### Named Volumes

```yaml
volumes:
  postgres-data:
  redis-data:
```

### Bind Mounts

```yaml
volumes:
  - ./data:/app/data
  - ./config:/app/config:ro
```

## Networking

### Custom Bridge Network

```yaml
networks:
  rootstream-net:
    driver: bridge
```

### Service Communication

Services communicate by name:
- `postgres:5432`
- `redis:6379`
- `rootstream-server:5000`

## Security

### Best Practices

1. **Non-root User**: Run as non-privileged user
2. **Read-only Filesystem**: Where possible
3. **No Secrets in Images**: Use environment variables or secrets management
4. **Minimal Base Image**: Reduce attack surface
5. **Security Scanning**: Scan images for vulnerabilities

### Scan Images

```bash
docker scan rootstream/server:latest
```

## Performance Optimization

### Multi-stage Builds

```dockerfile
# Build stage
FROM ubuntu:22.04 AS builder
# Build application

# Runtime stage
FROM ubuntu:22.04
# Copy only artifacts
```

### Layer Caching

- Order Dockerfile commands from least to most frequently changing
- Combine RUN commands where appropriate
- Use .dockerignore

### Resource Limits

```yaml
services:
  rootstream-server:
    deploy:
      resources:
        limits:
          cpus: '2'
          memory: 2G
        reservations:
          cpus: '0.5'
          memory: 512M
```

## Troubleshooting

### Container Won't Start

```bash
docker logs rootstream-server-1
docker inspect rootstream-server-1
```

### Network Issues

```bash
docker network ls
docker network inspect rootstream-net
```

### Volume Issues

```bash
docker volume ls
docker volume inspect rootstream_postgres-data
```

## CI/CD Integration

### GitHub Actions Example

```yaml
- name: Build Docker image
  run: docker build -t rootstream/server:${{ github.sha }} .

- name: Push to registry
  run: docker push rootstream/server:${{ github.sha }}
```

## License

MIT License - See root LICENSE file
