#ifndef GPU_MONITOR_H
#define GPU_MONITOR_H

#include "metrics_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gpu_monitor gpu_monitor_t;

// Initialize GPU monitor (auto-detect GPU vendor)
gpu_monitor_t* gpu_monitor_init(void);

// Update GPU metrics (call periodically)
void gpu_monitor_update(gpu_monitor_t* monitor);

// Get VRAM used in MB
uint32_t gpu_monitor_get_vram_used_mb(gpu_monitor_t* monitor);

// Get total VRAM in MB
uint32_t gpu_monitor_get_vram_total_mb(gpu_monitor_t* monitor);

// Get GPU utilization (0-100%)
uint8_t gpu_monitor_get_utilization(gpu_monitor_t* monitor);

// Get GPU temperature
uint8_t gpu_monitor_get_temperature(gpu_monitor_t* monitor);

// Check if thermal throttling
bool gpu_monitor_is_thermal_throttling(gpu_monitor_t* monitor);

// Get GPU statistics
void gpu_monitor_get_stats(gpu_monitor_t* monitor, gpu_metrics_t* out);

// Cleanup
void gpu_monitor_cleanup(gpu_monitor_t* monitor);

#ifdef __cplusplus
}
#endif

#endif // GPU_MONITOR_H
