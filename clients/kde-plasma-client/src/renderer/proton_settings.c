/**
 * @file proton_settings.c
 * @brief Proton settings implementation
 */

#include "proton_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SETTINGS_FILE ".rootstream_proton.conf"

void proton_settings_get_default(proton_settings_t *settings) {
    if (!settings) {
        return;
    }
    
    settings->enable_dxvk = true;
    settings->enable_vkd3d = true;
    settings->enable_async_shader_compile = true;
    settings->enable_dxvk_hud = false;
    settings->shader_cache_max_mb = 1024;
    strncpy(settings->preferred_directx_version, "auto", 
           sizeof(settings->preferred_directx_version) - 1);
}

int proton_settings_load(proton_settings_t *settings) {
    if (!settings) {
        return -1;
    }
    
    // Set defaults first
    proton_settings_get_default(settings);
    
    // Try to load from config file
    const char *home = getenv("HOME");
    if (!home) {
        return 0;  // Use defaults
    }
    
    char config_path[1024];
    snprintf(config_path, sizeof(config_path), "%s/%s", home, SETTINGS_FILE);
    
    FILE *f = fopen(config_path, "r");
    if (!f) {
        return 0;  // Use defaults
    }
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        
        // Parse key=value pairs
        char *equals = strchr(line, '=');
        if (!equals) {
            continue;
        }
        
        *equals = '\0';
        const char *key = line;
        const char *value = equals + 1;
        
        // Remove trailing newline from value
        char *newline = strchr(value, '\n');
        if (newline) {
            *newline = '\0';
        }
        
        // Parse settings
        if (strcmp(key, "enable_dxvk") == 0) {
            settings->enable_dxvk = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "enable_vkd3d") == 0) {
            settings->enable_vkd3d = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "enable_async_shader_compile") == 0) {
            settings->enable_async_shader_compile = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "enable_dxvk_hud") == 0) {
            settings->enable_dxvk_hud = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "shader_cache_max_mb") == 0) {
            settings->shader_cache_max_mb = atoi(value);
        } else if (strcmp(key, "preferred_directx_version") == 0) {
            strncpy(settings->preferred_directx_version, value,
                   sizeof(settings->preferred_directx_version) - 1);
        }
    }
    
    fclose(f);
    return 0;
}

int proton_settings_save(const proton_settings_t *settings) {
    if (!settings) {
        return -1;
    }
    
    const char *home = getenv("HOME");
    if (!home) {
        return -1;
    }
    
    char config_path[1024];
    snprintf(config_path, sizeof(config_path), "%s/%s", home, SETTINGS_FILE);
    
    FILE *f = fopen(config_path, "w");
    if (!f) {
        return -1;
    }
    
    fprintf(f, "# RootStream Proton Settings\n");
    fprintf(f, "enable_dxvk=%s\n", settings->enable_dxvk ? "true" : "false");
    fprintf(f, "enable_vkd3d=%s\n", settings->enable_vkd3d ? "true" : "false");
    fprintf(f, "enable_async_shader_compile=%s\n", 
           settings->enable_async_shader_compile ? "true" : "false");
    fprintf(f, "enable_dxvk_hud=%s\n", settings->enable_dxvk_hud ? "true" : "false");
    fprintf(f, "shader_cache_max_mb=%d\n", settings->shader_cache_max_mb);
    fprintf(f, "preferred_directx_version=%s\n", settings->preferred_directx_version);
    
    fclose(f);
    return 0;
}

int proton_settings_apply(const proton_settings_t *settings) {
    if (!settings) {
        return -1;
    }
    
    // Apply DXVK settings
    if (settings->enable_dxvk) {
        if (settings->enable_async_shader_compile) {
            setenv("DXVK_ASYNC", "1", 1);
        }
        
        if (settings->enable_dxvk_hud) {
            setenv("DXVK_HUD", "fps,frametimes,gpuload", 1);
        }
    }
    
    // Apply VKD3D settings
    if (settings->enable_vkd3d) {
        // VKD3D-specific settings could be applied here
    }
    
    return 0;
}
