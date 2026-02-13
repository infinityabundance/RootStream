#ifndef CPU_MONITOR_H
#define CPU_MONITOR_H

#include "metrics_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cpu_monitor cpu_monitor_t;

// Initialize CPU monitor
cpu_monitor_t* cpu_monitor_init(void);

// Update CPU metrics (call periodically)
void cpu_monitor_update(cpu_monitor_t* monitor);

// Get current CPU usage
uint8_t cpu_monitor_get_usage(cpu_monitor_t* monitor);

// Get per-core usage
uint8_t cpu_monitor_get_core_usage(cpu_monitor_t* monitor, int core);

// Get load average
float cpu_monitor_get_load_average(cpu_monitor_t* monitor);

// Get CPU temperature
uint8_t cpu_monitor_get_temperature(cpu_monitor_t* monitor);

// Check if thermal throttling
bool cpu_monitor_is_thermal_throttling(cpu_monitor_t* monitor);

// Get CPU statistics
void cpu_monitor_get_stats(cpu_monitor_t* monitor, cpu_metrics_t* out);

// Cleanup
void cpu_monitor_cleanup(cpu_monitor_t* monitor);

#ifdef __cplusplus
}
#endif

#endif // CPU_MONITOR_H
