#include "gpu_monitor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

typedef enum {
    GPU_VENDOR_UNKNOWN,
    GPU_VENDOR_NVIDIA,
    GPU_VENDOR_AMD,
    GPU_VENDOR_INTEL
} gpu_vendor_t;

struct gpu_monitor {
    gpu_vendor_t vendor;
    uint32_t vram_samples[METRICS_HISTORY_SIZE];
    uint8_t util_samples[METRICS_HISTORY_SIZE];
    uint8_t temp_samples[METRICS_HISTORY_SIZE];
    uint32_t sample_index;
    
    uint32_t vram_used_mb;
    uint32_t vram_total_mb;
    uint8_t utilization;
    uint8_t temperature;
    bool is_throttling;
    char gpu_model[64];
};

static gpu_vendor_t detect_gpu_vendor(void) {
    // Check for NVIDIA
    if (access("/usr/bin/nvidia-smi", X_OK) == 0) {
        return GPU_VENDOR_NVIDIA;
    }
    
    // Check for AMD
    if (access("/usr/bin/rocm-smi", X_OK) == 0) {
        return GPU_VENDOR_AMD;
    }
    
    // Check for Intel via DRM
    DIR* dir = opendir("/sys/class/drm");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, "card") && !strstr(entry->d_name, "-")) {
                char path[512];
                snprintf(path, sizeof(path), "/sys/class/drm/%s/device/vendor", entry->d_name);
                
                FILE* fp = fopen(path, "r");
                if (fp) {
                    char vendor[16];
                    if (fgets(vendor, sizeof(vendor), fp)) {
                        if (strstr(vendor, "0x8086")) {
                            fclose(fp);
                            closedir(dir);
                            return GPU_VENDOR_INTEL;
                        }
                    }
                    fclose(fp);
                }
            }
        }
        closedir(dir);
    }
    
    return GPU_VENDOR_UNKNOWN;
}

static void read_nvidia_stats(gpu_monitor_t* monitor) {
    FILE* fp = popen("nvidia-smi --query-gpu=memory.used,memory.total,utilization.gpu,temperature.gpu,name --format=csv,noheader,nounits 2>/dev/null", "r");
    if (!fp) return;
    
    char line[256];
    if (fgets(line, sizeof(line), fp)) {
        uint32_t mem_used, mem_total, util, temp;
        char name[64] = {0};
        
        // Parse CSV line
        if (sscanf(line, "%u, %u, %u, %u", &mem_used, &mem_total, &util, &temp) == 4) {
            monitor->vram_used_mb = mem_used;
            monitor->vram_total_mb = mem_total;
            monitor->utilization = (uint8_t)util;
            monitor->temperature = (uint8_t)temp;
            
            // Throttling check (NVIDIA typically throttles at 83-87C)
            monitor->is_throttling = (monitor->temperature >= 83);
            
            // Parse GPU name (after 4th comma)
            char* name_start = line;
            int comma_count = 0;
            while (*name_start && comma_count < 4) {
                if (*name_start == ',') comma_count++;
                name_start++;
            }
            if (*name_start) {
                // Skip leading spaces
                while (*name_start == ' ') name_start++;
                strncpy(monitor->gpu_model, name_start, sizeof(monitor->gpu_model) - 1);
                // Remove trailing newline
                char* newline = strchr(monitor->gpu_model, '\n');
                if (newline) *newline = '\0';
            }
        }
    }
    
    pclose(fp);
}

static void read_amd_stats(gpu_monitor_t* monitor) {
    // Try rocm-smi
    FILE* fp = popen("rocm-smi --showmeminfo vram --showuse --showtemp --json 2>/dev/null", "r");
    if (fp) {
        // Simplified parsing - in production, use proper JSON parser
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            uint32_t value;
            if (strstr(line, "VRAM Total Memory") && sscanf(line, "%*[^0-9]%u", &value) == 1) {
                monitor->vram_total_mb = value / (1024 * 1024); // Convert bytes to MB
            } else if (strstr(line, "VRAM Total Used Memory") && sscanf(line, "%*[^0-9]%u", &value) == 1) {
                monitor->vram_used_mb = value / (1024 * 1024);
            } else if (strstr(line, "GPU use") && sscanf(line, "%*[^0-9]%u", &value) == 1) {
                monitor->utilization = (uint8_t)value;
            } else if (strstr(line, "Temperature") && sscanf(line, "%*[^0-9]%u", &value) == 1) {
                monitor->temperature = (uint8_t)value;
            }
        }
        pclose(fp);
        
        // AMD throttles around 110C (junction temp)
        monitor->is_throttling = (monitor->temperature >= 100);
        strncpy(monitor->gpu_model, "AMD GPU", sizeof(monitor->gpu_model) - 1);
    }
}

static void read_intel_stats(gpu_monitor_t* monitor) {
    // Intel GPUs: Read from sysfs
    DIR* dir = opendir("/sys/class/drm");
    if (!dir) return;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, "card") && !strstr(entry->d_name, "-")) {
            char path[512];
            
            // Check if Intel GPU
            snprintf(path, sizeof(path), "/sys/class/drm/%s/device/vendor", entry->d_name);
            FILE* fp = fopen(path, "r");
            if (fp) {
                char vendor[16];
                if (fgets(vendor, sizeof(vendor), fp) && strstr(vendor, "0x8086")) {
                    fclose(fp);
                    
                    // Read GPU name
                    snprintf(path, sizeof(path), "/sys/class/drm/%s/device/uevent", entry->d_name);
                    fp = fopen(path, "r");
                    if (fp) {
                        char line[256];
                        while (fgets(line, sizeof(line), fp)) {
                            if (strstr(line, "PCI_ID=")) {
                                strncpy(monitor->gpu_model, "Intel GPU", sizeof(monitor->gpu_model) - 1);
                                break;
                            }
                        }
                        fclose(fp);
                    }
                    
                    // Intel doesn't expose VRAM easily in sysfs
                    monitor->vram_total_mb = 0;
                    monitor->vram_used_mb = 0;
                    monitor->utilization = 0;
                    monitor->temperature = 0;
                    monitor->is_throttling = false;
                    
                    closedir(dir);
                    return;
                }
                fclose(fp);
            }
        }
    }
    
    closedir(dir);
}

gpu_monitor_t* gpu_monitor_init(void) {
    gpu_monitor_t* monitor = (gpu_monitor_t*)calloc(1, sizeof(gpu_monitor_t));
    if (!monitor) {
        return NULL;
    }
    
    monitor->vendor = detect_gpu_vendor();
    strncpy(monitor->gpu_model, "Unknown GPU", sizeof(monitor->gpu_model) - 1);
    
    // Initial update
    gpu_monitor_update(monitor);
    
    return monitor;
}

void gpu_monitor_update(gpu_monitor_t* monitor) {
    if (!monitor) return;
    
    switch (monitor->vendor) {
        case GPU_VENDOR_NVIDIA:
            read_nvidia_stats(monitor);
            break;
        case GPU_VENDOR_AMD:
            read_amd_stats(monitor);
            break;
        case GPU_VENDOR_INTEL:
            read_intel_stats(monitor);
            break;
        default:
            break;
    }
    
    // Store samples
    monitor->vram_samples[monitor->sample_index] = monitor->vram_used_mb;
    monitor->util_samples[monitor->sample_index] = monitor->utilization;
    monitor->temp_samples[monitor->sample_index] = monitor->temperature;
    monitor->sample_index = (monitor->sample_index + 1) % METRICS_HISTORY_SIZE;
}

uint32_t gpu_monitor_get_vram_used_mb(gpu_monitor_t* monitor) {
    return monitor ? monitor->vram_used_mb : 0;
}

uint32_t gpu_monitor_get_vram_total_mb(gpu_monitor_t* monitor) {
    return monitor ? monitor->vram_total_mb : 0;
}

uint8_t gpu_monitor_get_utilization(gpu_monitor_t* monitor) {
    return monitor ? monitor->utilization : 0;
}

uint8_t gpu_monitor_get_temperature(gpu_monitor_t* monitor) {
    return monitor ? monitor->temperature : 0;
}

bool gpu_monitor_is_thermal_throttling(gpu_monitor_t* monitor) {
    return monitor ? monitor->is_throttling : false;
}

void gpu_monitor_get_stats(gpu_monitor_t* monitor, gpu_metrics_t* out) {
    if (!monitor || !out) return;
    
    memset(out, 0, sizeof(gpu_metrics_t));
    
    out->vram_used_mb = monitor->vram_used_mb;
    out->vram_total_mb = monitor->vram_total_mb;
    out->gpu_utilization = monitor->utilization;
    out->gpu_temp_celsius = monitor->temperature;
    out->thermal_throttling = monitor->is_throttling;
    strncpy(out->gpu_model, monitor->gpu_model, sizeof(out->gpu_model) - 1);
}

void gpu_monitor_cleanup(gpu_monitor_t* monitor) {
    if (monitor) {
        free(monitor);
    }
}
