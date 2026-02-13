/**
 * @file proton_detector.c
 * @brief Proton environment detection implementation
 */

#include "proton_detector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

// Helper to safely get environment variable
static const char* safe_getenv(const char *name) {
    const char *value = getenv(name);
    return value ? value : "";
}

// Helper to check if environment variable exists and is non-empty
static bool env_exists(const char *name) {
    const char *value = getenv(name);
    return value && value[0] != '\0';
}

bool proton_parse_version(const char *version_str, proton_version_t *version) {
    if (!version_str || !version) {
        return false;
    }
    
    memset(version, 0, sizeof(proton_version_t));
    
    // Parse major.minor.patch
    int parsed = sscanf(version_str, "%u.%u.%u", 
                       &version->major, &version->minor, &version->patch);
    
    if (parsed < 1) {
        return false;
    }
    
    // Check for suffix (e.g., "-GE", "-rc1")
    const char *dash = strchr(version_str, '-');
    if (dash) {
        strncpy(version->suffix, dash + 1, sizeof(version->suffix) - 1);
        version->suffix[sizeof(version->suffix) - 1] = '\0';
    }
    
    return true;
}

bool proton_detect_wine_prefix(char *prefix_path, size_t max_len) {
    const char *wine_prefix = getenv("WINEPREFIX");
    if (!wine_prefix) {
        wine_prefix = getenv("WINE_PREFIX");
    }
    
    if (wine_prefix && wine_prefix[0] != '\0') {
        strncpy(prefix_path, wine_prefix, max_len - 1);
        prefix_path[max_len - 1] = '\0';
        return true;
    }
    
    return false;
}

bool proton_detect_steam_app_id(char *app_id, size_t max_len) {
    // Check SteamAppId environment variable
    const char *steam_app = getenv("SteamAppId");
    if (!steam_app) {
        steam_app = getenv("STEAM_APP_ID");
    }
    
    if (steam_app && steam_app[0] != '\0') {
        strncpy(app_id, steam_app, max_len - 1);
        app_id[max_len - 1] = '\0';
        return true;
    }
    
    // Try to read from /proc/self/environ
    FILE *f = fopen("/proc/self/environ", "r");
    if (f) {
        char buf[4096];
        size_t nread = fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);
        
        buf[nread] = '\0';
        
        // Parse null-separated environment
        for (size_t i = 0; i < nread; ) {
            const char *entry = buf + i;
            size_t len = strlen(entry);
            
            if (strncmp(entry, "SteamAppId=", 11) == 0) {
                strncpy(app_id, entry + 11, max_len - 1);
                app_id[max_len - 1] = '\0';
                return true;
            }
            
            i += len + 1;
        }
    }
    
    return false;
}

bool proton_detect_dxvk_version(proton_version_t *version) {
    if (!version) {
        return false;
    }
    
    // Check DXVK_VERSION environment variable
    const char *dxvk_ver = getenv("DXVK_VERSION");
    if (dxvk_ver && dxvk_ver[0] != '\0') {
        return proton_parse_version(dxvk_ver, version);
    }
    
    // Try to detect from Wine DLLs
    const char *wine_prefix = getenv("WINEPREFIX");
    if (wine_prefix) {
        char dll_path[PATH_MAX];
        snprintf(dll_path, sizeof(dll_path), 
                "%s/drive_c/windows/system32/dxgi.dll", wine_prefix);
        
        if (access(dll_path, F_OK) == 0) {
            // DXVK is present, but version unknown
            // Set a default version
            version->major = 1;
            version->minor = 10;
            return true;
        }
    }
    
    return false;
}

bool proton_detect_vkd3d_version(proton_version_t *version) {
    if (!version) {
        return false;
    }
    
    // Check VKD3D_VERSION environment variable
    const char *vkd3d_ver = getenv("VKD3D_VERSION");
    if (vkd3d_ver && vkd3d_ver[0] != '\0') {
        return proton_parse_version(vkd3d_ver, version);
    }
    
    // Try to detect from Wine DLLs
    const char *wine_prefix = getenv("WINEPREFIX");
    if (wine_prefix) {
        char dll_path[PATH_MAX];
        snprintf(dll_path, sizeof(dll_path), 
                "%s/drive_c/windows/system32/d3d12.dll", wine_prefix);
        
        if (access(dll_path, F_OK) == 0) {
            // VKD3D is present, but version unknown
            version->major = 1;
            version->minor = 0;
            return true;
        }
    }
    
    return false;
}

bool proton_detect_directx_version(bool *has_d3d11, bool *has_d3d12) {
    if (!has_d3d11 || !has_d3d12) {
        return false;
    }
    
    *has_d3d11 = false;
    *has_d3d12 = false;
    
    const char *wine_prefix = getenv("WINEPREFIX");
    if (!wine_prefix) {
        return false;
    }
    
    // Check for D3D11 (DXVK)
    char d3d11_path[PATH_MAX];
    snprintf(d3d11_path, sizeof(d3d11_path), 
            "%s/drive_c/windows/system32/d3d11.dll", wine_prefix);
    *has_d3d11 = (access(d3d11_path, F_OK) == 0);
    
    // Check for D3D12 (VKD3D)
    char d3d12_path[PATH_MAX];
    snprintf(d3d12_path, sizeof(d3d12_path), 
            "%s/drive_c/windows/system32/d3d12.dll", wine_prefix);
    *has_d3d12 = (access(d3d12_path, F_OK) == 0);
    
    return *has_d3d11 || *has_d3d12;
}

bool proton_is_game_running(void) {
    // Check if we're running under Wine/Proton
    return env_exists("WINEPREFIX") || 
           env_exists("WINE_PREFIX") ||
           env_exists("PROTON_VERSION");
}

bool proton_detect(proton_info_t *info) {
    if (!info) {
        return false;
    }
    
    memset(info, 0, sizeof(proton_info_t));
    
    // Check for Proton version
    const char *proton_ver = getenv("PROTON_VERSION");
    if (proton_ver && proton_ver[0] != '\0') {
        strncpy(info->proton_version, proton_ver, sizeof(info->proton_version) - 1);
        info->proton_version[sizeof(info->proton_version) - 1] = '\0';
        info->is_running_under_proton = true;
    }
    
    // Alternative: Check for Wine (Proton is based on Wine)
    if (!info->is_running_under_proton && env_exists("WINEPREFIX")) {
        info->is_running_under_proton = true;
        strncpy(info->proton_version, "unknown", sizeof(info->proton_version) - 1);
    }
    
    if (!info->is_running_under_proton) {
        return false;
    }
    
    // Detect Wine prefix
    proton_detect_wine_prefix(info->wine_prefix_path, sizeof(info->wine_prefix_path));
    
    // Detect Steam App ID
    proton_detect_steam_app_id(info->steam_app_id, sizeof(info->steam_app_id));
    
    // Detect DXVK
    if (env_exists("DXVK_HUD") || env_exists("DXVK_VERSION")) {
        info->has_dxvk = proton_detect_dxvk_version(&info->dxvk_version);
        if (!info->has_dxvk) {
            // DXVK environment set but version unknown
            info->has_dxvk = true;
        }
        info->dxvk_async_enabled = env_exists("DXVK_ASYNC");
    }
    
    // Detect VKD3D
    if (env_exists("VKD3D_SHADER_DEBUG") || env_exists("VKD3D_VERSION")) {
        info->has_vkd3d = proton_detect_vkd3d_version(&info->vkd3d_version);
        if (!info->has_vkd3d) {
            // VKD3D environment set but version unknown
            info->has_vkd3d = true;
        }
        info->vkd3d_debug_enabled = env_exists("VKD3D_SHADER_DEBUG");
    }
    
    // Detect DirectX version
    proton_detect_directx_version(&info->has_d3d11, &info->has_d3d12);
    
    // Detect Proton security settings
    info->seccomp_enabled = env_exists("PROTON_USE_SECCOMP");
    
    // Get compatibility tool paths
    const char *compat_paths = getenv("STEAM_COMPAT_TOOL_PATHS");
    if (compat_paths) {
        strncpy(info->compat_tool_paths, compat_paths, sizeof(info->compat_tool_paths) - 1);
        info->compat_tool_paths[sizeof(info->compat_tool_paths) - 1] = '\0';
    }
    
    return true;
}

int proton_info_to_string(const proton_info_t *info, char *buf, size_t buf_len) {
    if (!info || !buf || buf_len == 0) {
        return 0;
    }
    
    int written = 0;
    
    if (!info->is_running_under_proton) {
        written = snprintf(buf, buf_len, "Not running under Proton");
        return written;
    }
    
    written = snprintf(buf, buf_len, 
        "Proton: %s\n"
        "Wine Prefix: %s\n"
        "Steam App ID: %s\n",
        info->proton_version[0] ? info->proton_version : "unknown",
        info->wine_prefix_path[0] ? info->wine_prefix_path : "not set",
        info->steam_app_id[0] ? info->steam_app_id : "unknown");
    
    if (written >= (int)buf_len) {
        return written;
    }
    
    if (info->has_dxvk) {
        int n = snprintf(buf + written, buf_len - written,
            "DXVK: %u.%u.%u%s (async: %s)\n",
            info->dxvk_version.major,
            info->dxvk_version.minor,
            info->dxvk_version.patch,
            info->dxvk_version.suffix[0] ? info->dxvk_version.suffix : "",
            info->dxvk_async_enabled ? "yes" : "no");
        written += n;
    }
    
    if (written >= (int)buf_len) {
        return written;
    }
    
    if (info->has_vkd3d) {
        int n = snprintf(buf + written, buf_len - written,
            "VKD3D: %u.%u.%u%s (debug: %s)\n",
            info->vkd3d_version.major,
            info->vkd3d_version.minor,
            info->vkd3d_version.patch,
            info->vkd3d_version.suffix[0] ? info->vkd3d_version.suffix : "",
            info->vkd3d_debug_enabled ? "yes" : "no");
        written += n;
    }
    
    if (written >= (int)buf_len) {
        return written;
    }
    
    if (info->has_d3d11 || info->has_d3d12) {
        int n = snprintf(buf + written, buf_len - written,
            "DirectX: %s%s%s\n",
            info->has_d3d11 ? "D3D11" : "",
            (info->has_d3d11 && info->has_d3d12) ? ", " : "",
            info->has_d3d12 ? "D3D12" : "");
        written += n;
    }
    
    return written;
}
