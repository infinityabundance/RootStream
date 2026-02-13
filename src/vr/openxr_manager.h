#ifndef OPENXR_MANAGER_H
#define OPENXR_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// OpenXR forward declarations (to avoid requiring OpenXR headers)
typedef struct XrInstance_T* XrInstance;
typedef struct XrSession_T* XrSession;
typedef uint64_t XrSystemId;
typedef int32_t XrEnvironmentBlendMode;

// Eye enumeration
typedef enum {
    XR_EYE_LEFT = 0,
    XR_EYE_RIGHT = 1
} XREye;

// Vector and quaternion types
typedef struct {
    float x, y, z;
} XrVector3f;

typedef struct {
    float x, y, z, w;
} XrQuaternionf;

typedef struct {
    XrQuaternionf orientation;
    XrVector3f position;
} XrPosef;

typedef struct {
    float x, y;
} XrVector2f;

// XR state structure
typedef struct {
    XrPosef headPose;
    XrPosef leftEyePose;
    XrPosef rightEyePose;
    XrPosef leftHandPose;
    XrPosef rightHandPose;
    XrQuaternionf headOrientation;
    XrVector3f headLinearVelocity;
    XrVector3f headAngularVelocity;
    uint64_t timestamp_us;
} XRState;

// Input state structure
typedef struct {
    float leftTrigger;
    float rightTrigger;
    bool buttonA, buttonB, buttonX, buttonY;
    bool buttonGrip, buttonMenu;
    XrVector2f leftThumbstick;
    XrVector2f rightThumbstick;
    XrPosef leftControllerPose;
    XrPosef rightControllerPose;
} XRInputState;

// OpenXR Manager structure
typedef struct OpenXRManager OpenXRManager;

// Initialization and cleanup
OpenXRManager* openxr_manager_create(void);
int openxr_manager_init(OpenXRManager *manager);
void openxr_manager_cleanup(OpenXRManager *manager);
void openxr_manager_destroy(OpenXRManager *manager);

// Session management
int openxr_manager_create_session(OpenXRManager *manager);
int openxr_manager_begin_frame(OpenXRManager *manager);
int openxr_manager_end_frame(OpenXRManager *manager);

// View and projection matrices (stored as 4x4 float arrays)
int openxr_manager_get_eye_projection(OpenXRManager *manager, XREye eye, float projection[16]);
int openxr_manager_get_eye_view(OpenXRManager *manager, XREye eye, float view[16]);

// Head tracking
XRState openxr_manager_get_tracking_data(OpenXRManager *manager);
bool openxr_manager_is_tracking_active(OpenXRManager *manager);

// Input handling
XRInputState openxr_manager_get_input(OpenXRManager *manager);

// Haptic feedback
int openxr_manager_vibrate_controller(OpenXRManager *manager, XREye hand, 
                                     float intensity, float duration_ms);

// Frame swapchain
uint32_t openxr_manager_acquire_swapchain_image(OpenXRManager *manager);
int openxr_manager_release_swapchain_image(OpenXRManager *manager);

// System information
int openxr_manager_get_recommended_resolution(OpenXRManager *manager, 
                                              uint32_t *width, uint32_t *height);

#ifdef __cplusplus
}
#endif

#endif // OPENXR_MANAGER_H
