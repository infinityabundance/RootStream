/**
 * @file proton_detector.h
 * @brief Proton environment detection for RootStream
 * 
 * Detects when running under Proton/Wine and identifies:
 * - Proton version and Wine prefix
 * - DXVK availability and version (D3D11 games)
 * - VKD3D availability and version (D3D12 games)
 * - Steam App ID
 * - DirectX version used by game
 */

#ifndef PROTON_DETECTOR_H
#define PROTON_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROTON_VERSION_MAX 64
#define STEAM_APP_ID_MAX 32

/**
 * Proton version structure
 */
typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    char suffix[16];  // e.g., "GE", "rc1"
} proton_version_t;

/**
 * Proton environment information
 */
typedef struct {
    bool is_running_under_proton;
    char proton_version[PROTON_VERSION_MAX];
    char steam_app_id[STEAM_APP_ID_MAX];
    char wine_prefix_path[PATH_MAX];
    
    // DXVK info (D3D11)
    bool has_dxvk;
    proton_version_t dxvk_version;
    bool dxvk_async_enabled;
    
    // VKD3D info (D3D12)
    bool has_vkd3d;
    proton_version_t vkd3d_version;
    bool vkd3d_debug_enabled;
    
    // DirectX version detection
    bool has_d3d11;
    bool has_d3d12;
    
    // Additional Proton settings
    bool seccomp_enabled;
    char compat_tool_paths[PATH_MAX];
} proton_info_t;

/**
 * Detect Proton environment
 * 
 * Checks environment variables and system state to determine if
 * running under Proton/Wine compatibility layer.
 * 
 * @param info Output structure to fill with Proton information
 * @return true if Proton detected, false otherwise
 */
bool proton_detect(proton_info_t *info);

/**
 * Detect Steam App ID
 * 
 * @param app_id Output buffer for App ID
 * @param max_len Maximum length of app_id buffer
 * @return true if App ID found, false otherwise
 */
bool proton_detect_steam_app_id(char *app_id, size_t max_len);

/**
 * Detect Wine prefix path
 * 
 * @param prefix_path Output buffer for Wine prefix
 * @param max_len Maximum length of prefix_path buffer
 * @return true if Wine prefix found, false otherwise
 */
bool proton_detect_wine_prefix(char *prefix_path, size_t max_len);

/**
 * Detect DXVK version
 * 
 * @param version Output structure for version
 * @return true if DXVK detected, false otherwise
 */
bool proton_detect_dxvk_version(proton_version_t *version);

/**
 * Detect VKD3D version
 * 
 * @param version Output structure for version
 * @return true if VKD3D detected, false otherwise
 */
bool proton_detect_vkd3d_version(proton_version_t *version);

/**
 * Check if a Proton game is currently running
 * 
 * @return true if Proton game process detected, false otherwise
 */
bool proton_is_game_running(void);

/**
 * Detect DirectX version used by current process
 * 
 * @param has_d3d11 Output: true if D3D11 detected
 * @param has_d3d12 Output: true if D3D12 detected
 * @return true if any DirectX version detected, false otherwise
 */
bool proton_detect_directx_version(bool *has_d3d11, bool *has_d3d12);

/**
 * Parse version string into structured version
 * 
 * Parses version strings like "8.3", "9.0-GE", "1.10.2"
 * 
 * @param version_str Version string to parse
 * @param version Output structure
 * @return true on success, false on parse error
 */
bool proton_parse_version(const char *version_str, proton_version_t *version);

/**
 * Get human-readable Proton info string
 * 
 * @param info Proton information
 * @param buf Output buffer
 * @param buf_len Buffer length
 * @return Number of characters written (excluding null terminator)
 */
int proton_info_to_string(const proton_info_t *info, char *buf, size_t buf_len);

#ifdef __cplusplus
}
#endif

#endif /* PROTON_DETECTOR_H */
