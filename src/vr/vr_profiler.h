#ifndef VR_PROFILER_H
#define VR_PROFILER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Frame metrics
typedef struct {
    float frametime_ms;
    float apptime_ms;
    float rendertime_ms;
    float latency_ms;
    float fps;
    float gpu_utilization;
    float cpu_utilization;
    float memory_usage_mb;
    uint64_t timestamp_us;
} VRFrameMetrics;

// Performance issue
typedef struct {
    char issue[256];
    char recommendation[512];
    float severity;  // 0.0 - 1.0
} VRPerformanceIssue;

// VR Profiler structure
typedef struct VRProfiler VRProfiler;

// Creation and initialization
VRProfiler* vr_profiler_create(void);
int vr_profiler_init(VRProfiler *profiler);
void vr_profiler_cleanup(VRProfiler *profiler);
void vr_profiler_destroy(VRProfiler *profiler);

// Recording metrics
int vr_profiler_record_frame(VRProfiler *profiler, const VRFrameMetrics *metrics);

// Getting metrics
VRFrameMetrics vr_profiler_get_average_metrics(VRProfiler *profiler, uint32_t frameWindow);
VRFrameMetrics vr_profiler_get_current_metrics(VRProfiler *profiler);

// Performance analysis
int vr_profiler_detect_issues(VRProfiler *profiler, VRPerformanceIssue *issues, 
                              uint32_t maxIssues, uint32_t *issueCount);

// Recommendations
bool vr_profiler_should_enable_foveated_rendering(VRProfiler *profiler);
int vr_profiler_adjust_quality(VRProfiler *profiler, float targetFps, float *recommendedScale);

// Reporting
int vr_profiler_generate_report(VRProfiler *profiler, char *report, size_t reportSize);

#ifdef __cplusplus
}
#endif

#endif // VR_PROFILER_H
