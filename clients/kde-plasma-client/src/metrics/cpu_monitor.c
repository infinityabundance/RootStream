#include "cpu_monitor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

struct cpu_monitor {
    uint64_t prev_total[16];
    uint64_t prev_idle[16];
    uint8_t cpu_samples[16][METRICS_HISTORY_SIZE];
    float load_samples[METRICS_HISTORY_SIZE];
    uint32_t sample_index;
    uint8_t num_cores;
    uint8_t current_cpu_usage;
    uint8_t current_core_usage[16];
    float current_load_average;
    uint8_t current_temp;
    bool is_throttling;
};

cpu_monitor_t* cpu_monitor_init(void) {
    cpu_monitor_t* monitor = (cpu_monitor_t*)calloc(1, sizeof(cpu_monitor_t));
    if (!monitor) {
        return NULL;
    }
    
    // Detect number of cores
    monitor->num_cores = (uint8_t)sysconf(_SC_NPROCESSORS_ONLN);
    if (monitor->num_cores > 16) {
        monitor->num_cores = 16;
    }
    
    // Initial CPU stats read
    cpu_monitor_update(monitor);
    
    return monitor;
}

static void read_cpu_stats(cpu_monitor_t* monitor) {
    FILE* fp = fopen("/proc/stat", "r");
    if (!fp) return;
    
    char line[256];
    uint64_t total_idle = 0;
    uint64_t total_sum = 0;
    int core_idx = 0;
    
    while (fgets(line, sizeof(line), fp) && core_idx <= monitor->num_cores) {
        if (strncmp(line, "cpu", 3) != 0) break;
        
        // Skip "cpu " (aggregate) if we want per-core
        if (line[3] == ' ' && core_idx == 0) {
            // Parse aggregate CPU
            uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
            sscanf(line, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
                   &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
            
            uint64_t total = user + nice + system + idle + iowait + irq + softirq + steal;
            
            if (monitor->prev_total[0] > 0) {
                uint64_t total_delta = total - monitor->prev_total[0];
                uint64_t idle_delta = idle - monitor->prev_idle[0];
                
                if (total_delta > 0) {
                    monitor->current_cpu_usage = (uint8_t)(100 * (total_delta - idle_delta) / total_delta);
                }
            }
            
            monitor->prev_total[0] = total;
            monitor->prev_idle[0] = idle;
        } else if (line[3] >= '0' && line[3] <= '9') {
            // Per-core stats
            int cpu_num;
            uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
            sscanf(line, "cpu%d %lu %lu %lu %lu %lu %lu %lu %lu",
                   &cpu_num, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
            
            if (cpu_num >= 0 && cpu_num < monitor->num_cores) {
                uint64_t total = user + nice + system + idle + iowait + irq + softirq + steal;
                int idx = cpu_num + 1; // Offset by 1 since idx 0 is aggregate
                
                if (monitor->prev_total[idx] > 0) {
                    uint64_t total_delta = total - monitor->prev_total[idx];
                    uint64_t idle_delta = idle - monitor->prev_idle[idx];
                    
                    if (total_delta > 0) {
                        monitor->current_core_usage[cpu_num] = 
                            (uint8_t)(100 * (total_delta - idle_delta) / total_delta);
                    }
                }
                
                monitor->prev_total[idx] = total;
                monitor->prev_idle[idx] = idle;
            }
            core_idx++;
        }
    }
    
    fclose(fp);
}

static void read_load_average(cpu_monitor_t* monitor) {
    FILE* fp = fopen("/proc/loadavg", "r");
    if (!fp) return;
    
    float load1, load5, load15;
    if (fscanf(fp, "%f %f %f", &load1, &load5, &load15) == 3) {
        monitor->current_load_average = load1;
    }
    
    fclose(fp);
}

static void read_cpu_temperature(cpu_monitor_t* monitor) {
    // Try common thermal zone paths
    const char* thermal_paths[] = {
        "/sys/class/thermal/thermal_zone0/temp",
        "/sys/class/thermal/thermal_zone1/temp",
        "/sys/class/hwmon/hwmon0/temp1_input",
        "/sys/class/hwmon/hwmon1/temp1_input",
        NULL
    };
    
    for (int i = 0; thermal_paths[i]; i++) {
        FILE* fp = fopen(thermal_paths[i], "r");
        if (fp) {
            int temp_millicelsius;
            if (fscanf(fp, "%d", &temp_millicelsius) == 1) {
                monitor->current_temp = (uint8_t)(temp_millicelsius / 1000);
                fclose(fp);
                
                // Check for throttling (typically > 85C)
                monitor->is_throttling = (monitor->current_temp > 85);
                return;
            }
            fclose(fp);
        }
    }
    
    monitor->current_temp = 0;
    monitor->is_throttling = false;
}

void cpu_monitor_update(cpu_monitor_t* monitor) {
    if (!monitor) return;
    
    read_cpu_stats(monitor);
    read_load_average(monitor);
    read_cpu_temperature(monitor);
    
    // Store samples
    monitor->cpu_samples[0][monitor->sample_index] = monitor->current_cpu_usage;
    monitor->load_samples[monitor->sample_index] = monitor->current_load_average;
    monitor->sample_index = (monitor->sample_index + 1) % METRICS_HISTORY_SIZE;
}

uint8_t cpu_monitor_get_usage(cpu_monitor_t* monitor) {
    return monitor ? monitor->current_cpu_usage : 0;
}

uint8_t cpu_monitor_get_core_usage(cpu_monitor_t* monitor, int core) {
    if (!monitor || core < 0 || core >= monitor->num_cores) return 0;
    return monitor->current_core_usage[core];
}

float cpu_monitor_get_load_average(cpu_monitor_t* monitor) {
    return monitor ? monitor->current_load_average : 0.0f;
}

uint8_t cpu_monitor_get_temperature(cpu_monitor_t* monitor) {
    return monitor ? monitor->current_temp : 0;
}

bool cpu_monitor_is_thermal_throttling(cpu_monitor_t* monitor) {
    return monitor ? monitor->is_throttling : false;
}

void cpu_monitor_get_stats(cpu_monitor_t* monitor, cpu_metrics_t* out) {
    if (!monitor || !out) return;
    
    memset(out, 0, sizeof(cpu_metrics_t));
    
    out->cpu_usage_percent = monitor->current_cpu_usage;
    out->num_cores = monitor->num_cores;
    out->load_average = monitor->current_load_average;
    out->cpu_temp_celsius = monitor->current_temp;
    out->thermal_throttling = monitor->is_throttling;
    
    for (int i = 0; i < monitor->num_cores; i++) {
        out->core_usage[i] = monitor->current_core_usage[i];
    }
}

void cpu_monitor_cleanup(cpu_monitor_t* monitor) {
    if (monitor) {
        free(monitor);
    }
}
