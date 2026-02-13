# RootStream Network Optimization System (PHASE 20)

## Overview

The RootStream Network Optimization System provides comprehensive real-time network monitoring and adaptive optimization for smooth streaming across variable network conditions.

## Architecture

```
┌────────────────────────────────────────────────────────────┐
│                  RootStream Network Stack                  │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │         Adaptive Bitrate Controller                 │  │
│  │  - Monitor bandwidth                                │  │
│  │  - Estimate available capacity                      │  │
│  │  - Adjust video encoding bitrate                    │  │
│  │  - Switch codec/resolution                          │  │
│  └─────────────────────────────────────────────────────┘  │
│                        │                                    │
│  ┌─────────────────────▼─────────────────────────────────┐ │
│  │      Network Quality Monitor                         │ │
│  │  - RTT measurement (EWMA)                           │ │
│  │  - Packet loss detection                            │ │
│  │  - Jitter calculation                               │ │
│  │  - Bandwidth estimation (AIMD)                      │ │
│  │  - Congestion detection (5 levels)                  │ │
│  └─────────────────────┬─────────────────────────────────┘ │
│                        │                                    │
│  ┌─────────────────────▼─────────────────────────────────┐ │
│  │      QoS/Traffic Prioritization                     │ │
│  │  - Classify packets (video/audio/control)          │ │
│  │  - Set DSCP/TOS fields                             │ │
│  │  - Prioritize key frames                           │ │
│  │  - Rate shaping                                    │ │
│  └─────────────────────┬─────────────────────────────────┘ │
│                        │                                    │
│  ┌─────────────────────▼─────────────────────────────────┐ │
│  │      Loss Recovery & Jitter Buffer                  │ │
│  │  - NACK-based retransmission                        │ │
│  │  - XOR-based FEC                                    │ │
│  │  - Packet jitter buffer                            │ │
│  │  - Adaptive delay management                       │ │
│  └────────────────────────────────────────────────────────┘ │
│                                                             │
└────────────────────────────────────────────────────────────┘
```

## Components

### 1. Network Monitor (`network_monitor.h/c`)

Real-time monitoring of network conditions:

- **RTT Measurement**: Uses EWMA (Exponential Weighted Moving Average) for smooth RTT tracking
- **Packet Loss**: Tracks packet loss percentage over a sliding window
- **Jitter**: Calculates RTT variance to measure jitter
- **Bandwidth Estimation**: Estimates available bandwidth using delivery rate
- **Congestion Detection**: 5-level congestion classification:
  - `EXCELLENT`: RTT <20ms, loss <0.1%
  - `GOOD`: RTT <50ms, loss <1%
  - `FAIR`: RTT <100ms, loss <2%
  - `POOR`: RTT <200ms, loss <5%
  - `CRITICAL`: RTT >200ms, loss >5%

**Usage:**
```c
network_monitor_t *monitor = network_monitor_create();

// Record packet sent
network_monitor_record_packet_sent(monitor, seq_num, timestamp_us);

// Record packet ack
network_monitor_record_packet_ack(monitor, seq_num, timestamp_us);

// Get current conditions
network_conditions_t cond = network_monitor_get_conditions(monitor);
printf("RTT: %ums, Loss: %.2f%%, Bandwidth: %u Mbps\n", 
       cond.rtt_ms, cond.packet_loss_percent, cond.bandwidth_mbps);
```

### 2. Adaptive Bitrate Controller (`adaptive_bitrate.h/c`)

Dynamically adjusts streaming quality based on network conditions:

- **Profile Management**: Maintains sorted list of quality profiles
- **Smart Switching**: Uses hysteresis to prevent rapid switching
- **Configurable Thresholds**: Customizable upgrade/downgrade thresholds
- **Default Profiles**: 480p @ 30fps to 4K @ 30fps

**Usage:**
```c
abr_controller_t *abr = abr_controller_create(monitor);

// Add quality profiles
abr_controller_add_profile(abr, 1500, 1280, 720, 30, "H.264", "fast");
abr_controller_add_profile(abr, 5000, 1920, 1080, 30, "H.264", "medium");

// Get recommended profile
const bitrate_profile_t *profile = abr_controller_get_recommended_profile(abr);
printf("Recommended: %ux%u @ %u fps, %u kbps\n",
       profile->width, profile->height, profile->fps, profile->bitrate_kbps);
```

### 3. QoS Manager (`qos_manager.h/c`)

Traffic prioritization and classification:

- **4 Priority Levels**: Critical, High, Medium, Low
- **DSCP Marking**: Sets appropriate DSCP values
  - Video keyframes: DSCP 46 (EF - Expedited Forwarding)
  - Video P-frames: DSCP 34 (AF41 - Assured Forwarding)
  - Audio: DSCP 26 (AF31)
  - Control: DSCP 0 (CS0)
- **Smart Drop Policy**: Priority-based packet dropping under congestion

### 4. Bandwidth Estimator (`bandwidth_estimator.h/c`)

AIMD (Additive Increase Multiplicative Decrease) algorithm:

- **Slow Start**: Exponential increase until threshold
- **Congestion Avoidance**: Linear increase
- **Fast Recovery**: Multiplicative decrease on congestion
- **Congestion Detection**: Based on packet loss and RTT

### 5. Socket Tuning (`socket_tuning.h/c`)

Optimizes TCP/UDP socket parameters:

- **Congestion Control**: CUBIC, BBR, Reno, BIC
- **Low Latency Mode**: TCP_NODELAY, small buffers (256KB)
- **Throughput Mode**: Large buffers (2MB)
- **ECN Support**: Explicit Congestion Notification
- **MTU Discovery**: Path MTU discovery

### 6. Jitter Buffer (`jitter_buffer.h/c`)

Smooths out network jitter:

- **Packet Buffering**: Holds packets for target delay
- **Adaptive Delay**: Adjusts delay based on RTT and jitter
- **Sequence Ordering**: Ensures correct packet ordering
- **Loss Tracking**: Monitors buffered packet loss rate

### 7. Loss Recovery (`loss_recovery.h/c`)

Recovers from packet loss:

- **NACK (Negative Acknowledgment)**: Request retransmission
- **XOR-based FEC**: Simple forward error correction
- **Adaptive Strategy**: Switches between NACK and FEC based on loss rate
- **Recovery Statistics**: Tracks retransmissions and recoveries

### 8. Load Balancer (`load_balancer.h/c`)

Multi-stream bandwidth allocation:

- **Fair Share**: Equal bandwidth distribution
- **Per-Stream Tracking**: Monitors bitrate, loss, RTT per stream
- **Dynamic Allocation**: Reallocates bandwidth as streams join/leave

### 9. Network Config (`network_config.h/c`)

Configuration management:

- **File-based Config**: Load/save configuration
- **Default Settings**: Sensible defaults for all parameters
- **Runtime Updates**: Dynamic configuration changes

### 10. Network Optimizer (`network_optimizer.h/c`)

Main coordinator that integrates all components:

- **Periodic Optimization**: Runs optimization cycle
- **Callback System**: Notifies on state changes
- **Diagnostics**: JSON export of network statistics
- **Easy Integration**: Simple API for embedding

**Usage:**
```c
// Create optimizer
network_optimizer_t *optimizer = network_optimizer_create();

// Setup callbacks
network_optimizer_callbacks_t callbacks = {
    .on_bitrate_changed = on_bitrate_change,
    .on_congestion_detected = on_congestion,
    .user_data = ctx
};

network_optimizer_init(optimizer, &callbacks);
network_optimizer_setup_default_profiles(optimizer);

// In main loop
network_optimizer_optimize(optimizer);

// Get diagnostics
char *json = network_optimizer_get_diagnostics_json(optimizer);
printf("%s\n", json);
free(json);
```

## Integration with RootStream

The network optimization system is designed to integrate seamlessly with the existing RootStream codebase:

1. **Network Monitor** tracks all sent/received packets
2. **ABR Controller** recommends bitrate adjustments to encoder
3. **QoS Manager** classifies packets before sending
4. **Jitter Buffer** smooths incoming video/audio packets
5. **Loss Recovery** handles packet loss transparently

## Testing

Comprehensive unit tests are provided:

```bash
# Compile tests
gcc -o tests/unit/test_network_optimization \
    tests/unit/test_network_optimization.c \
    src/network/*.c -lpthread -lm

# Run tests
./tests/unit/test_network_optimization
```

**Test Coverage:**
- Network monitor RTT measurement
- Adaptive bitrate profile selection
- Bandwidth estimator AIMD algorithm
- QoS packet classification
- Network optimizer integration
- **Result: 13/13 tests passing (100%)**

## Performance Characteristics

- **Low Overhead**: Minimal CPU usage (<1%)
- **Thread-Safe**: All components use pthread mutexes
- **Memory Efficient**: Fixed-size buffers, no dynamic allocation in hot paths
- **Scalable**: Supports up to 16 concurrent streams

## Configuration Example

Default configuration provides excellent results for most scenarios:

```c
network_config_t config = network_config_get_default();
// min_bitrate: 500 kbps
// max_bitrate: 50 Mbps
// jitter_buffer: 100ms
// enable_qos: true
// enable_fec: true
```

## Future Enhancements

Potential areas for expansion:

1. **Reed-Solomon FEC**: More robust error correction (requires libzfec)
2. **Network Diagnostics Tool**: Bandwidth testing, latency measurement
3. **Machine Learning**: AI-based bitrate prediction
4. **Multi-path Support**: Use multiple network interfaces
5. **WebRTC Integration**: Compatibility with WebRTC protocols

## References

- **AIMD Algorithm**: RFC 5681 (TCP Congestion Control)
- **QoS/DSCP**: RFC 2474 (Differentiated Services)
- **FEC**: RFC 5109 (RTP Payload Format for Generic FEC)
- **BBR Congestion Control**: IETF Draft

## License

MIT License - See root LICENSE file
