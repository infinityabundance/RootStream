#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H

#include "metrics_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct memory_monitor memory_monitor_t;

// Initialize memory monitor
memory_monitor_t* memory_monitor_init(void);

// Update memory metrics (call periodically)
void memory_monitor_update(memory_monitor_t* monitor);

// Get used RAM in MB
uint32_t memory_monitor_get_ram_used_mb(memory_monitor_t* monitor);

// Get total RAM in MB
uint32_t memory_monitor_get_ram_total_mb(memory_monitor_t* monitor);

// Get used swap in MB
uint32_t memory_monitor_get_swap_used_mb(memory_monitor_t* monitor);

// Get cache size in MB
uint32_t memory_monitor_get_cache_mb(memory_monitor_t* monitor);

// Get RAM usage percentage
uint8_t memory_monitor_get_ram_usage_percent(memory_monitor_t* monitor);

// Get memory statistics
void memory_monitor_get_stats(memory_monitor_t* monitor, memory_metrics_t* out);

// Cleanup
void memory_monitor_cleanup(memory_monitor_t* monitor);

#ifdef __cplusplus
}
#endif

#endif // MEMORY_MONITOR_H
