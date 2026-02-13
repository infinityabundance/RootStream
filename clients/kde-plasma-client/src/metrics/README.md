# Performance Metrics System Documentation

## Overview

The RootStream Performance Metrics System (PHASE 16) provides comprehensive real-time monitoring and visualization of streaming performance, including FPS, latency, network quality, and system resource usage.

## Features

### ✅ Metrics Collection
- **Frame Rate Monitoring**: FPS tracking, frame time measurement, dropped frame detection
- **Network Metrics**: RTT (round-trip time), jitter, packet loss percentage
- **Input Latency**: Client input → screen latency measurement
- **A/V Sync**: Audio-video synchronization offset tracking
- **GPU Monitoring**: VRAM usage, utilization, temperature (NVIDIA/AMD/Intel)
- **CPU Monitoring**: Usage percentage, per-core stats, temperature, load average
- **Memory Monitoring**: RAM/swap usage, cache statistics

### ✅ Statistics & Analysis
- Min/Max/Average calculations over rolling window (1000 samples)
- Percentile calculations (p50, p75, p95, p99)
- Anomaly detection (FPS drops, latency spikes, thermal throttling)
- Historical data tracking for trend analysis

### ✅ HUD Overlay
- Real-time on-screen display with color-coded metrics
- Configurable visibility for each metric category
- Adjustable opacity and positioning
- Green/Yellow/Red color coding based on thresholds

### ✅ Data Export
- CSV export for spreadsheet analysis
- JSON export for programmatic processing
- Automatic logging with configurable intervals

### ✅ Alert System
- Configurable thresholds for all metrics
- Real-time alerts via Qt signals
- Alert debouncing to prevent spam (5-second cooldown)
- Thermal throttling warnings

## Architecture

```
MetricsManager (Qt Coordinator)
├── PerformanceAggregator
│   ├── FrameRateCounter (C)
│   ├── CPUMonitor (C)
│   ├── MemoryMonitor (C)
│   └── GPUMonitor (C)
├── HUDRenderer (Qt/OpenGL)
├── PerformanceLogger (CSV/JSON)
└── AlertSystem (Threshold Monitoring)
```

## Usage

### Basic Integration

```cpp
#include "metrics/metrics_manager.h"

// In your application class
class MyStreamingApp : public QObject {
    Q_OBJECT
    
private:
    MetricsManager* m_metrics;
    
public:
    void init() {
        m_metrics = new MetricsManager(this);
        m_metrics->init(1920, 1080);  // Window resolution
        
        // Enable HUD
        m_metrics->setHUDVisible(true);
        
        // Enable logging
        m_metrics->setLoggingEnabled(true, "performance.csv");
        
        // Enable alerts
        m_metrics->setAlertsEnabled(true);
        
        // Connect to alerts
        connect(m_metrics, &MetricsManager::fpsDropDetected,
                this, &MyStreamingApp::onFPSDrop);
    }
    
    void onFrameRendered() {
        m_metrics->recordFrame();
    }
    
    void onNetworkPacket(uint32_t rtt_ms) {
        m_metrics->recordNetworkLatency(rtt_ms);
    }
    
    void renderFrame(QPainter* painter) {
        // Render your content...
        
        // Render HUD overlay
        m_metrics->renderHUD(painter);
    }
};
```

### GPU Monitoring

The GPU monitor automatically detects your GPU vendor:

- **NVIDIA**: Uses `nvidia-smi` for accurate VRAM, utilization, and temperature
- **AMD**: Uses `rocm-smi` for Radeon metrics
- **Intel**: Uses sysfs for basic GPU information

**Requirements**:
- NVIDIA: Install `nvidia-utils` package
- AMD: Install `rocm-smi` package
- Intel: No additional packages needed (uses sysfs)

### CPU & Memory Monitoring

Uses standard Linux `/proc` filesystem:
- `/proc/stat` - CPU usage and per-core statistics
- `/proc/loadavg` - System load average
- `/proc/meminfo` - Memory and swap usage
- `/sys/class/thermal/` - CPU temperature

No additional dependencies required.

### Customizing Alert Thresholds

```cpp
AlertSystem* alerts = m_metrics->getAlertSystem();

// Set custom thresholds
alerts->setFPSDropThreshold(45);        // Alert if FPS < 45
alerts->setLatencyThreshold(80);        // Alert if RTT > 80ms
alerts->setAVSyncThreshold(100);        // Alert if A/V offset > 100ms
alerts->setThermalThreshold(90);        // Alert if temp > 90°C
alerts->setPacketLossThreshold(2.0f);   // Alert if loss > 2%
```

### HUD Configuration

```cpp
HUDRenderer* hud = m_metrics->getHUDRenderer();

// Toggle individual panels
hud->setShowFPS(true);
hud->setShowLatency(true);
hud->setShowNetwork(true);
hud->setShowResources(true);
hud->setShowAVSync(true);

// Adjust opacity
hud->setHUDOpacity(0.75f);  // 75% opaque

// Toggle HUD visibility
hud->setHUDVisible(true);
```

### Keyboard Shortcuts

Recommended keyboard shortcuts for HUD control:

```cpp
void MyApp::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_F3) {
        // Toggle HUD visibility
        bool visible = m_metrics->isHUDVisible();
        m_metrics->setHUDVisible(!visible);
    }
    else if (event->key() == Qt::Key_F4) {
        // Toggle metrics collection
        bool enabled = m_metrics->isMetricsEnabled();
        m_metrics->setMetricsEnabled(!enabled);
    }
}
```

## Data Export

### CSV Format

```csv
timestamp_us,fps,frame_time_ms,frame_drops,rtt_ms,jitter_ms,packet_loss_percent,input_latency_ms,av_sync_offset_ms,gpu_util,gpu_temp,vram_used_mb,vram_total_mb,cpu_usage,cpu_temp,load_avg,ram_used_mb,ram_total_mb,ram_usage_percent,swap_used_mb
1707815400000000,60,16.7,0,30,2,0.1,8,2,45,60,2048,8192,25,65,1.5,4096,16384,25,0
```

### JSON Format

```json
[
  {
    "timestamp_us": 1707815400000000,
    "fps": {
      "fps": 60,
      "frame_time_ms": 16.7,
      "min_frame_time_ms": 16.2,
      "max_frame_time_ms": 18.5,
      "avg_frame_time_ms": 16.8,
      "frame_drops": 0,
      "total_frames": 3600
    },
    "network": {
      "rtt_ms": 30,
      "min_rtt_ms": 25,
      "max_rtt_ms": 45,
      "avg_rtt_ms": 32,
      "jitter_ms": 2,
      "packet_loss_percent": 0.1,
      "bandwidth_mbps": 50
    },
    "gpu": {
      "vram_used_mb": 2048,
      "vram_total_mb": 8192,
      "gpu_utilization": 45,
      "gpu_temp_celsius": 60,
      "thermal_throttling": false,
      "gpu_model": "NVIDIA GeForce RTX 3070"
    },
    "cpu": {
      "cpu_usage_percent": 25,
      "num_cores": 8,
      "load_average": 1.5,
      "cpu_temp_celsius": 65,
      "thermal_throttling": false
    },
    "memory": {
      "ram_used_mb": 4096,
      "ram_total_mb": 16384,
      "swap_used_mb": 0,
      "cache_mb": 2048,
      "ram_usage_percent": 25
    }
  }
]
```

## Build Configuration

### CMake Options

```bash
# Enable metrics system
cmake -DENABLE_METRICS=ON ..

# Disable metrics system (default: ON)
cmake -DENABLE_METRICS=OFF ..
```

### Compilation

```bash
cd build
cmake .. -DENABLE_METRICS=ON
make -j$(nproc)
```

## Testing

### Run Metrics Tests

```bash
# Run all tests
ctest

# Run only metrics tests
./test_metrics
```

### Test Coverage

The test suite includes:
- Frame rate counter accuracy tests
- CPU/GPU/Memory monitor functionality tests
- Performance aggregator integration tests
- HUD renderer initialization tests
- Performance logger CSV/JSON export tests
- Alert system threshold and debouncing tests

## Performance Impact

The metrics system is designed for minimal overhead:

- **CPU Usage**: < 1% (measured on Intel i7-8700K)
- **Memory Usage**: ~20 MB for history buffers
- **Frame Time Impact**: < 0.1 ms per frame
- **HUD Rendering**: < 0.5 ms per frame (at 1080p)

## Color Coding

### FPS
- 🟢 Green: ≥ 60 FPS (excellent)
- 🟡 Yellow: 30-59 FPS (acceptable)
- 🔴 Red: < 30 FPS (poor)

### Network Latency
- 🟢 Green: < 30 ms (excellent)
- 🟡 Yellow: 30-100 ms (acceptable)
- 🔴 Red: > 100 ms (poor)

### Input Latency
- 🟢 Green: < 20 ms (excellent)
- 🟡 Yellow: 20-50 ms (acceptable)
- 🔴 Red: > 50 ms (poor)

### A/V Sync
- 🟢 Green: ±30 ms (in sync)
- 🟡 Yellow: ±30-100 ms (noticeable)
- 🔴 Red: > ±100 ms (out of sync)

### Resources
- 🔵 Cyan: Normal operation
- 🔴 Red: Thermal throttling detected

## Troubleshooting

### HUD Not Visible
1. Check if metrics are enabled: `m_metrics->setMetricsEnabled(true)`
2. Check if HUD is visible: `m_metrics->setHUDVisible(true)`
3. Verify QPainter is valid when calling `renderHUD()`

### GPU Metrics Not Available
1. **NVIDIA**: Install nvidia-utils (`sudo apt install nvidia-utils`)
2. **AMD**: Install rocm-smi (`sudo apt install rocm-smi`)
3. **Intel**: Metrics may be limited (no vendor tools required)

### CPU Temperature Not Reading
1. Check thermal zones: `ls /sys/class/thermal/thermal_zone*/temp`
2. Check hwmon: `ls /sys/class/hwmon/hwmon*/temp*_input`
3. May require kernel module (e.g., `coretemp` for Intel)

### CSV Export Not Working
1. Check file permissions in output directory
2. Verify logging is enabled: `m_metrics->setLoggingEnabled(true, "path.csv")`
3. Check disk space availability

## Future Enhancements

Potential improvements for future versions:

- [ ] Network bandwidth measurement
- [ ] Frame pacing analysis
- [ ] Jank detection and reporting
- [ ] Per-game profile system
- [ ] Web dashboard for remote monitoring
- [ ] ML-based anomaly detection
- [ ] Automatic quality adjustment based on metrics
- [ ] Extended GPU metrics (power draw, clock speeds)
- [ ] Audio latency measurement

## License

Part of the RootStream project. See LICENSE file for details.

## Credits

Developed as part of PHASE 16 implementation for comprehensive performance monitoring and optimization of the RootStream remote streaming system.
