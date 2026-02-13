/**
 * @file proton_settings.h
 * @brief Proton renderer configuration and settings
 */

#ifndef PROTON_SETTINGS_H
#define PROTON_SETTINGS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Proton settings structure
 */
typedef struct {
    bool enable_dxvk;
    bool enable_vkd3d;
    bool enable_async_shader_compile;
    bool enable_dxvk_hud;
    int shader_cache_max_mb;
    char preferred_directx_version[8];  // "11", "12", or "auto"
} proton_settings_t;

/**
 * Load Proton settings from configuration file
 * 
 * @param settings Output structure for settings
 * @return 0 on success, -1 on failure
 */
int proton_settings_load(proton_settings_t *settings);

/**
 * Save Proton settings to configuration file
 * 
 * @param settings Settings to save
 * @return 0 on success, -1 on failure
 */
int proton_settings_save(const proton_settings_t *settings);

/**
 * Apply settings to environment
 * 
 * Sets appropriate environment variables based on settings.
 * 
 * @param settings Settings to apply
 * @return 0 on success, -1 on failure
 */
int proton_settings_apply(const proton_settings_t *settings);

/**
 * Get default settings
 * 
 * @param settings Output structure for default settings
 */
void proton_settings_get_default(proton_settings_t *settings);

#ifdef __cplusplus
}
#endif

#endif /* PROTON_SETTINGS_H */
