#include "memory_monitor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct memory_monitor {
    uint32_t ram_samples[METRICS_HISTORY_SIZE];
    uint32_t swap_samples[METRICS_HISTORY_SIZE];
    uint32_t sample_index;
    uint32_t ram_total_mb;
    uint32_t ram_used_mb;
    uint32_t swap_total_mb;
    uint32_t swap_used_mb;
    uint32_t cache_mb;
    uint8_t ram_usage_percent;
};

memory_monitor_t* memory_monitor_init(void) {
    memory_monitor_t* monitor = (memory_monitor_t*)calloc(1, sizeof(memory_monitor_t));
    if (!monitor) {
        return NULL;
    }
    
    // Initial memory stats read
    memory_monitor_update(monitor);
    
    return monitor;
}

static void read_memory_stats(memory_monitor_t* monitor) {
    FILE* fp = fopen("/proc/meminfo", "r");
    if (!fp) return;
    
    char line[256];
    uint32_t mem_total = 0, mem_free = 0, mem_available = 0;
    uint32_t buffers = 0, cached = 0, slab = 0;
    uint32_t swap_total = 0, swap_free = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        uint32_t value;
        
        if (sscanf(line, "MemTotal: %u kB", &value) == 1) {
            mem_total = value;
        } else if (sscanf(line, "MemFree: %u kB", &value) == 1) {
            mem_free = value;
        } else if (sscanf(line, "MemAvailable: %u kB", &value) == 1) {
            mem_available = value;
        } else if (sscanf(line, "Buffers: %u kB", &value) == 1) {
            buffers = value;
        } else if (sscanf(line, "Cached: %u kB", &value) == 1) {
            cached = value;
        } else if (sscanf(line, "Slab: %u kB", &value) == 1) {
            slab = value;
        } else if (sscanf(line, "SwapTotal: %u kB", &value) == 1) {
            swap_total = value;
        } else if (sscanf(line, "SwapFree: %u kB", &value) == 1) {
            swap_free = value;
        }
    }
    
    fclose(fp);
    
    // Convert to MB
    monitor->ram_total_mb = mem_total / 1024;
    
    // Use MemAvailable if available, otherwise calculate
    if (mem_available > 0) {
        monitor->ram_used_mb = (mem_total - mem_available) / 1024;
    } else {
        monitor->ram_used_mb = (mem_total - mem_free - buffers - cached - slab) / 1024;
    }
    
    monitor->swap_total_mb = swap_total / 1024;
    monitor->swap_used_mb = (swap_total - swap_free) / 1024;
    monitor->cache_mb = (cached + buffers) / 1024;
    
    // Calculate percentage
    if (monitor->ram_total_mb > 0) {
        monitor->ram_usage_percent = (uint8_t)((monitor->ram_used_mb * 100) / monitor->ram_total_mb);
    } else {
        monitor->ram_usage_percent = 0;
    }
}

void memory_monitor_update(memory_monitor_t* monitor) {
    if (!monitor) return;
    
    read_memory_stats(monitor);
    
    // Store samples
    monitor->ram_samples[monitor->sample_index] = monitor->ram_used_mb;
    monitor->swap_samples[monitor->sample_index] = monitor->swap_used_mb;
    monitor->sample_index = (monitor->sample_index + 1) % METRICS_HISTORY_SIZE;
}

uint32_t memory_monitor_get_ram_used_mb(memory_monitor_t* monitor) {
    return monitor ? monitor->ram_used_mb : 0;
}

uint32_t memory_monitor_get_ram_total_mb(memory_monitor_t* monitor) {
    return monitor ? monitor->ram_total_mb : 0;
}

uint32_t memory_monitor_get_swap_used_mb(memory_monitor_t* monitor) {
    return monitor ? monitor->swap_used_mb : 0;
}

uint32_t memory_monitor_get_cache_mb(memory_monitor_t* monitor) {
    return monitor ? monitor->cache_mb : 0;
}

uint8_t memory_monitor_get_ram_usage_percent(memory_monitor_t* monitor) {
    return monitor ? monitor->ram_usage_percent : 0;
}

void memory_monitor_get_stats(memory_monitor_t* monitor, memory_metrics_t* out) {
    if (!monitor || !out) return;
    
    memset(out, 0, sizeof(memory_metrics_t));
    
    out->ram_used_mb = monitor->ram_used_mb;
    out->ram_total_mb = monitor->ram_total_mb;
    out->swap_used_mb = monitor->swap_used_mb;
    out->cache_mb = monitor->cache_mb;
    out->ram_usage_percent = monitor->ram_usage_percent;
}

void memory_monitor_cleanup(memory_monitor_t* monitor) {
    if (monitor) {
        free(monitor);
    }
}
