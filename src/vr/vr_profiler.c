#include "vr_profiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_FRAME_HISTORY 300  // 5 seconds at 60 FPS

struct VRProfiler {
    VRFrameMetrics history[MAX_FRAME_HISTORY];
    uint32_t historySize;
    uint32_t historyIndex;
    VRFrameMetrics currentMetrics;
    bool initialized;
};

VRProfiler* vr_profiler_create(void) {
    VRProfiler *profiler = (VRProfiler*)calloc(1, sizeof(VRProfiler));
    if (!profiler) {
        fprintf(stderr, "Failed to allocate VRProfiler\n");
        return NULL;
    }
    
    profiler->initialized = false;
    profiler->historySize = 0;
    profiler->historyIndex = 0;
    
    return profiler;
}

int vr_profiler_init(VRProfiler *profiler) {
    if (!profiler) {
        return -1;
    }
    
    memset(&profiler->currentMetrics, 0, sizeof(VRFrameMetrics));
    memset(profiler->history, 0, sizeof(profiler->history));
    
    profiler->initialized = true;
    
    printf("VR profiler initialized\n");
    
    return 0;
}

int vr_profiler_record_frame(VRProfiler *profiler, const VRFrameMetrics *metrics) {
    if (!profiler || !profiler->initialized || !metrics) {
        return -1;
    }
    
    profiler->currentMetrics = *metrics;
    
    // Add to history
    profiler->history[profiler->historyIndex] = *metrics;
    profiler->historyIndex = (profiler->historyIndex + 1) % MAX_FRAME_HISTORY;
    if (profiler->historySize < MAX_FRAME_HISTORY) {
        profiler->historySize++;
    }
    
    return 0;
}

VRFrameMetrics vr_profiler_get_average_metrics(VRProfiler *profiler, uint32_t frameWindow) {
    VRFrameMetrics avg = {0};
    
    if (!profiler || !profiler->initialized || profiler->historySize == 0) {
        return avg;
    }
    
    if (frameWindow == 0 || frameWindow > profiler->historySize) {
        frameWindow = profiler->historySize;
    }
    
    for (uint32_t i = 0; i < frameWindow; i++) {
        uint32_t idx = (profiler->historyIndex + MAX_FRAME_HISTORY - frameWindow + i) % MAX_FRAME_HISTORY;
        avg.frametime_ms += profiler->history[idx].frametime_ms;
        avg.apptime_ms += profiler->history[idx].apptime_ms;
        avg.rendertime_ms += profiler->history[idx].rendertime_ms;
        avg.latency_ms += profiler->history[idx].latency_ms;
        avg.fps += profiler->history[idx].fps;
        avg.gpu_utilization += profiler->history[idx].gpu_utilization;
        avg.cpu_utilization += profiler->history[idx].cpu_utilization;
        avg.memory_usage_mb += profiler->history[idx].memory_usage_mb;
    }
    
    float scale = 1.0f / (float)frameWindow;
    avg.frametime_ms *= scale;
    avg.apptime_ms *= scale;
    avg.rendertime_ms *= scale;
    avg.latency_ms *= scale;
    avg.fps *= scale;
    avg.gpu_utilization *= scale;
    avg.cpu_utilization *= scale;
    avg.memory_usage_mb *= scale;
    
    return avg;
}

VRFrameMetrics vr_profiler_get_current_metrics(VRProfiler *profiler) {
    if (!profiler || !profiler->initialized) {
        VRFrameMetrics empty = {0};
        return empty;
    }
    
    return profiler->currentMetrics;
}

int vr_profiler_detect_issues(VRProfiler *profiler, VRPerformanceIssue *issues, 
                              uint32_t maxIssues, uint32_t *issueCount) {
    if (!profiler || !profiler->initialized || !issues || !issueCount) {
        return -1;
    }
    
    *issueCount = 0;
    
    VRFrameMetrics avg = vr_profiler_get_average_metrics(profiler, 60);
    
    // Check for low FPS
    if (avg.fps < 80.0f && *issueCount < maxIssues) {
        snprintf(issues[*issueCount].issue, sizeof(issues[*issueCount].issue),
                "Low FPS: %.1f (target: 90+)", avg.fps);
        snprintf(issues[*issueCount].recommendation, sizeof(issues[*issueCount].recommendation),
                "Consider reducing render resolution or enabling foveated rendering");
        issues[*issueCount].severity = (90.0f - avg.fps) / 90.0f;
        (*issueCount)++;
    }
    
    // Check for high latency
    if (avg.latency_ms > 20.0f && *issueCount < maxIssues) {
        snprintf(issues[*issueCount].issue, sizeof(issues[*issueCount].issue),
                "High latency: %.1f ms (target: <20ms)", avg.latency_ms);
        snprintf(issues[*issueCount].recommendation, sizeof(issues[*issueCount].recommendation),
                "Enable prediction and reduce render pipeline stages");
        issues[*issueCount].severity = (avg.latency_ms - 20.0f) / 20.0f;
        (*issueCount)++;
    }
    
    // Check for high GPU utilization
    if (avg.gpu_utilization > 95.0f && *issueCount < maxIssues) {
        snprintf(issues[*issueCount].issue, sizeof(issues[*issueCount].issue),
                "GPU bottleneck: %.1f%% utilization", avg.gpu_utilization);
        snprintf(issues[*issueCount].recommendation, sizeof(issues[*issueCount].recommendation),
                "Reduce render resolution or simplify rendering pipeline");
        issues[*issueCount].severity = (avg.gpu_utilization - 95.0f) / 5.0f;
        (*issueCount)++;
    }
    
    // Check for high memory usage
    if (avg.memory_usage_mb > 4096.0f && *issueCount < maxIssues) {
        snprintf(issues[*issueCount].issue, sizeof(issues[*issueCount].issue),
                "High memory usage: %.0f MB", avg.memory_usage_mb);
        snprintf(issues[*issueCount].recommendation, sizeof(issues[*issueCount].recommendation),
                "Reduce texture quality or implement texture streaming");
        issues[*issueCount].severity = (avg.memory_usage_mb - 4096.0f) / 4096.0f;
        (*issueCount)++;
    }
    
    return 0;
}

bool vr_profiler_should_enable_foveated_rendering(VRProfiler *profiler) {
    if (!profiler || !profiler->initialized) {
        return false;
    }
    
    VRFrameMetrics avg = vr_profiler_get_average_metrics(profiler, 60);
    
    // Enable foveated rendering if FPS is below target or GPU is highly utilized
    return (avg.fps < 85.0f || avg.gpu_utilization > 85.0f);
}

int vr_profiler_adjust_quality(VRProfiler *profiler, float targetFps, float *recommendedScale) {
    if (!profiler || !profiler->initialized || !recommendedScale) {
        return -1;
    }
    
    VRFrameMetrics avg = vr_profiler_get_average_metrics(profiler, 60);
    
    // Simple adaptive quality algorithm
    if (avg.fps < targetFps * 0.9f) {
        // Reduce quality
        *recommendedScale = 0.9f;
    } else if (avg.fps > targetFps * 1.1f && avg.gpu_utilization < 70.0f) {
        // Increase quality
        *recommendedScale = 1.1f;
    } else {
        // Maintain current quality
        *recommendedScale = 1.0f;
    }
    
    return 0;
}

int vr_profiler_generate_report(VRProfiler *profiler, char *report, size_t reportSize) {
    if (!profiler || !profiler->initialized || !report || reportSize == 0) {
        return -1;
    }
    
    VRFrameMetrics avg = vr_profiler_get_average_metrics(profiler, 60);
    
    int written = snprintf(report, reportSize,
        "VR Performance Report (60 frame average):\n"
        "  FPS: %.1f\n"
        "  Frame Time: %.2f ms\n"
        "  Render Time: %.2f ms\n"
        "  Latency: %.2f ms\n"
        "  GPU Utilization: %.1f%%\n"
        "  CPU Utilization: %.1f%%\n"
        "  Memory Usage: %.0f MB\n",
        avg.fps,
        avg.frametime_ms,
        avg.rendertime_ms,
        avg.latency_ms,
        avg.gpu_utilization,
        avg.cpu_utilization,
        avg.memory_usage_mb
    );
    
    return (written > 0 && (size_t)written < reportSize) ? 0 : -1;
}

void vr_profiler_cleanup(VRProfiler *profiler) {
    if (!profiler) {
        return;
    }
    
    profiler->initialized = false;
    profiler->historySize = 0;
    
    printf("VR profiler cleaned up\n");
}

void vr_profiler_destroy(VRProfiler *profiler) {
    if (!profiler) {
        return;
    }
    
    vr_profiler_cleanup(profiler);
    free(profiler);
}
