#ifndef METRICS_TYPES_H
#define METRICS_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define METRICS_HISTORY_SIZE 1000    // Rolling window
#define METRICS_PERCENTILE_BUCKETS 100

// Frame rate metrics
typedef struct {
    uint32_t fps;                   // Current frames per second
    float frame_time_ms;            // Current frame time
    float min_frame_time_ms;        // Minimum in window
    float max_frame_time_ms;        // Maximum in window
    float avg_frame_time_ms;        // Average in window
    uint32_t frame_drops;           // Missed frames
    uint64_t total_frames;          // Total frames rendered
} frame_rate_metrics_t;

// Network latency metrics
typedef struct {
    uint32_t rtt_ms;                // Round-trip time (current)
    uint32_t min_rtt_ms;            // Minimum RTT
    uint32_t max_rtt_ms;            // Maximum RTT
    uint32_t avg_rtt_ms;            // Average RTT
    uint32_t jitter_ms;             // RTT variance
    float packet_loss_percent;      // Lost packets (%)
    uint32_t bandwidth_mbps;        // Current bandwidth usage
} network_metrics_t;

// Input latency metrics
typedef struct {
    uint32_t input_latency_ms;      // Client input → screen
    uint32_t min_input_latency_ms;
    uint32_t max_input_latency_ms;
    uint32_t avg_input_latency_ms;
    uint32_t input_queue_depth;     // Pending inputs
    uint32_t total_inputs;          // Processed inputs
} input_metrics_t;

// Audio-video sync metrics
typedef struct {
    int32_t av_sync_offset_ms;      // Positive = audio ahead
    int32_t min_offset_ms;
    int32_t max_offset_ms;
    int32_t avg_offset_ms;
    uint32_t sync_corrections;      // Playback speed adjustments
    uint32_t audio_underruns;       // Buffer starvation events
} av_sync_metrics_t;

// GPU metrics
typedef struct {
    uint32_t vram_used_mb;          // VRAM usage
    uint32_t vram_total_mb;         // Total VRAM
    uint8_t gpu_utilization;        // GPU load (0-100%)
    uint8_t gpu_temp_celsius;       // GPU temperature
    bool thermal_throttling;        // GPU throttling?
    char gpu_model[64];             // GPU name
} gpu_metrics_t;

// CPU metrics
typedef struct {
    uint8_t cpu_usage_percent;      // Overall CPU load
    uint8_t core_usage[16];         // Per-core usage
    uint8_t num_cores;              // Number of cores
    float load_average;             // 1-minute load average
    uint8_t cpu_temp_celsius;       // CPU temperature
    bool thermal_throttling;        // CPU throttling?
} cpu_metrics_t;

// Memory metrics
typedef struct {
    uint32_t ram_used_mb;           // Used RAM
    uint32_t ram_total_mb;          // Total RAM
    uint32_t swap_used_mb;          // Used swap
    uint32_t cache_mb;              // Cache size
    uint8_t ram_usage_percent;      // RAM load (%)
} memory_metrics_t;

// Aggregated metrics snapshot
typedef struct {
    uint64_t timestamp_us;
    frame_rate_metrics_t fps;
    network_metrics_t network;
    input_metrics_t input;
    av_sync_metrics_t av_sync;
    gpu_metrics_t gpu;
    cpu_metrics_t cpu;
    memory_metrics_t memory;
} metrics_snapshot_t;

// Percentile stats
typedef struct {
    uint32_t p50;
    uint32_t p75;
    uint32_t p95;
    uint32_t p99;
} percentile_stats_t;

#ifdef __cplusplus
}
#endif

#endif // METRICS_TYPES_H
