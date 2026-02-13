# Monitoring & Health Checks

This module provides comprehensive health monitoring and alerting capabilities for RootStream infrastructure.

## Overview

The monitoring module tracks:
- System health (CPU, memory, disk usage)
- Service availability (API, database, cache, storage)
- Active connections
- Custom metrics
- Alert thresholds

## Components

### HealthCheckManager

Main class for health monitoring and alerting.

**Files**: `health_check.h`, `health_check.cpp`

## Features

### Health Monitoring
- Overall system health status
- Component-specific health checks
- Resource utilization tracking
- Uptime monitoring

### Alerting
- Configurable threshold alerts
- Multiple alert channels support
- Alert deduplication

### Metrics Collection
- CPU usage percentage
- Memory utilization
- Disk usage
- Network connections
- Custom application metrics

## Usage

### Initialize

```cpp
#include "health_check.h"

HealthCheckManager healthCheck;
healthCheck.init();
```

### Get Overall Health

```cpp
HealthStatus status = healthCheck.getOverallHealth();

std::cout << "API Healthy: " << status.api_healthy << std::endl;
std::cout << "Database Healthy: " << status.database_healthy << std::endl;
std::cout << "Cache Healthy: " << status.cache_healthy << std::endl;
std::cout << "Storage Healthy: " << status.storage_healthy << std::endl;
std::cout << "CPU Usage: " << status.cpu_usage << "%" << std::endl;
std::cout << "Memory Usage: " << status.memory_usage << "%" << std::endl;
std::cout << "Disk Usage: " << status.disk_usage << "%" << std::endl;
std::cout << "Active Connections: " << status.active_connections << std::endl;
std::cout << "Uptime: " << status.uptime_seconds << "s" << std::endl;
```

### Check Individual Components

```cpp
// Database connectivity
bool dbHealthy = healthCheck.checkDatabaseConnectivity();

// Cache connectivity
bool cacheHealthy = healthCheck.checkCacheConnectivity();

// Storage availability
bool storageHealthy = healthCheck.checkStorageConnectivity();

// Quick health check
bool allHealthy = healthCheck.isHealthy();
```

### Configure Alerts

```cpp
// Alert if CPU usage exceeds 80%
healthCheck.setHealthAlert("cpu", 80.0f);

// Alert if memory usage exceeds 90%
healthCheck.setHealthAlert("memory", 90.0f);

// Alert if disk usage exceeds 85%
healthCheck.setHealthAlert("disk", 85.0f);

// Check alerts periodically
healthCheck.checkAlerts();
```

### Remove Alerts

```cpp
healthCheck.removeHealthAlert("cpu");
```

### Get Metrics

```cpp
auto metrics = healthCheck.getMetrics();

for (const auto &metric : metrics) {
    std::cout << metric.first << ": " << metric.second << std::endl;
}
```

## Health Endpoints

Implement HTTP endpoints for health checks:

### /health

Returns overall health status (200 OK if healthy, 503 if unhealthy).

```json
{
  "status": "healthy",
  "api": true,
  "database": true,
  "cache": true,
  "storage": true,
  "cpu_usage": 45.2,
  "memory_usage": 62.8,
  "disk_usage": 43.1,
  "active_connections": 142,
  "uptime_seconds": 86400
}
```

### /ready

Returns readiness status (200 OK if ready to accept traffic).

```json
{
  "status": "ready",
  "checks": {
    "database": "ok",
    "cache": "ok",
    "storage": "ok"
  }
}
```

### /metrics

Returns Prometheus-compatible metrics.

```text
# HELP rootstream_cpu_usage CPU usage percentage
# TYPE rootstream_cpu_usage gauge
rootstream_cpu_usage 45.2

# HELP rootstream_memory_usage Memory usage percentage
# TYPE rootstream_memory_usage gauge
rootstream_memory_usage 62.8

# HELP rootstream_active_connections Active network connections
# TYPE rootstream_active_connections gauge
rootstream_active_connections 142
```

## Integration

### Kubernetes Probes

```yaml
livenessProbe:
  httpGet:
    path: /health
    port: 5001
  initialDelaySeconds: 30
  periodSeconds: 10

readinessProbe:
  httpGet:
    path: /ready
    port: 5001
  initialDelaySeconds: 5
  periodSeconds: 5
```

### Prometheus

Configure Prometheus to scrape metrics:

```yaml
scrape_configs:
  - job_name: 'rootstream'
    static_configs:
      - targets: ['rootstream-server:5001']
    metrics_path: '/metrics'
    scrape_interval: 15s
```

### Grafana Dashboard

Import or create dashboard with:
- CPU/Memory/Disk usage graphs
- Connection count over time
- Service health status
- Alert history

## Alerting Channels

### Email Alerts

```cpp
void HealthCheckManager::triggerAlert(const std::string &service, 
                                     const std::string &message) {
    // Send email via SMTP
    sendEmail("ops@rootstream.io", "Alert: " + service, message);
}
```

### Slack Integration

```bash
curl -X POST -H 'Content-type: application/json' \
  --data '{"text":"Alert: CPU usage exceeded threshold"}' \
  https://hooks.slack.com/services/YOUR/WEBHOOK/URL
```

### PagerDuty

```bash
curl -X POST https://events.pagerduty.com/v2/enqueue \
  -H 'Content-Type: application/json' \
  -d '{
    "routing_key": "YOUR_ROUTING_KEY",
    "event_action": "trigger",
    "payload": {
      "summary": "CPU usage alert",
      "severity": "warning",
      "source": "rootstream-server"
    }
  }'
```

## Metrics Collection

### System Metrics

Collected from:
- `/proc/stat` - CPU usage
- `/proc/meminfo` - Memory usage
- `df` command - Disk usage
- `netstat` - Network connections

### Application Metrics

Track custom metrics:
```cpp
// Example: Track stream count
healthCheck.publishMetric("active_streams", streamCount);
healthCheck.publishMetric("total_bytes_transferred", bytesCount);
```

## Monitoring Best Practices

1. **Set Realistic Thresholds**: Don't alert on every spike
2. **Monitor Trends**: Look at metrics over time
3. **Alert on Symptoms**: Alert on user-facing issues
4. **Runbook Links**: Include remediation steps in alerts
5. **Alert Fatigue**: Avoid too many alerts
6. **Regular Testing**: Test alert mechanisms

## CloudWatch Integration

### Publish Metrics

```cpp
aws cloudwatch put-metric-data \
  --namespace RootStream \
  --metric-name CPUUsage \
  --value 45.2 \
  --unit Percent
```

### Create Alarms

```bash
aws cloudwatch put-metric-alarm \
  --alarm-name rootstream-high-cpu \
  --alarm-description "CPU usage exceeded 80%" \
  --metric-name CPUUsage \
  --namespace RootStream \
  --statistic Average \
  --period 300 \
  --threshold 80 \
  --comparison-operator GreaterThanThreshold
```

## Datadog Integration

```cpp
// Send metrics to Datadog
statsd.gauge("rootstream.cpu.usage", cpuUsage);
statsd.gauge("rootstream.memory.usage", memUsage);
statsd.increment("rootstream.connections.active");
```

## Troubleshooting

### High CPU Usage

1. Check process list: `top`
2. Review application logs
3. Look for CPU-intensive operations
4. Consider scaling horizontally

### High Memory Usage

1. Check for memory leaks
2. Review memory allocation patterns
3. Consider vertical scaling
4. Enable memory profiling

### Database Connection Issues

1. Check database server status
2. Verify connection string
3. Check network connectivity
4. Review connection pool settings

### Cache Connection Issues

1. Verify Redis is running
2. Check Redis logs
3. Test connection: `redis-cli ping`
4. Review firewall rules

## Performance Considerations

- Cache health check results (don't check every request)
- Use async health checks where possible
- Set reasonable check intervals
- Minimize overhead of monitoring

## Testing

### Mock Health Checks

```cpp
class MockHealthCheck : public HealthCheckManager {
public:
    bool mockHealthy = true;
    
    bool isHealthy() override {
        return mockHealthy;
    }
};
```

### Load Testing

Monitor metrics during load tests:
```bash
ab -n 10000 -c 100 http://localhost:5001/health
```

## Future Enhancements

- [ ] Distributed tracing integration (Jaeger, Zipkin)
- [ ] Custom metric aggregation
- [ ] Anomaly detection
- [ ] Predictive alerting
- [ ] Mobile app notifications
- [ ] Integration with incident management systems

## License

MIT License - See root LICENSE file
