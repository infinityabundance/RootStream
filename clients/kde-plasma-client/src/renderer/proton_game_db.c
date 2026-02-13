/**
 * @file proton_game_db.c
 * @brief Game compatibility database implementation
 */

#include "proton_game_db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Known game workarounds database
 */
static const game_workaround_t KNOWN_GAMES[] = {
    {
        .steam_app_id = 570,
        .game_name = "Dota 2",
        .issue_description = "Shader compilation stalls",
        .env_override = "DXVK_ASYNC=1",
        .dxvk_version_min = "1.10",
        .vkd3d_version_min = NULL,
        .requires_async_compile = true,
        .requires_low_latency_mode = true,
    },
    {
        .steam_app_id = 730,
        .game_name = "Counter-Strike: Global Offensive",
        .issue_description = "Frame pacing issues",
        .env_override = "DXVK_ASYNC=1;DXVK_HUD=fps",
        .dxvk_version_min = "1.9",
        .vkd3d_version_min = NULL,
        .requires_async_compile = true,
        .requires_low_latency_mode = true,
    },
    {
        .steam_app_id = 271590,
        .game_name = "Grand Theft Auto V",
        .issue_description = "High memory usage",
        .env_override = "DXVK_ASYNC=1",
        .dxvk_version_min = "1.10",
        .vkd3d_version_min = NULL,
        .requires_async_compile = true,
        .requires_low_latency_mode = false,
    },
    {
        .steam_app_id = 377160,
        .game_name = "Fallout 4",
        .issue_description = "D3D11 performance",
        .env_override = "DXVK_ASYNC=1;DXVK_STATE_CACHE=1",
        .dxvk_version_min = "1.10",
        .vkd3d_version_min = NULL,
        .requires_async_compile = true,
        .requires_low_latency_mode = false,
    },
    {
        .steam_app_id = 1174180,
        .game_name = "Red Dead Redemption 2",
        .issue_description = "Requires VKD3D for D3D12",
        .env_override = "VKD3D_CONFIG=dxr",
        .dxvk_version_min = NULL,
        .vkd3d_version_min = "1.2",
        .requires_async_compile = false,
        .requires_low_latency_mode = false,
    },
    // Add more games as needed
};

static const int KNOWN_GAMES_COUNT = sizeof(KNOWN_GAMES) / sizeof(KNOWN_GAMES[0]);

int proton_game_db_lookup(uint32_t steam_app_id,
                         const game_workaround_t **workarounds,
                         int max_count) {
    if (!workarounds || max_count <= 0) {
        return 0;
    }
    
    int found = 0;
    for (int i = 0; i < KNOWN_GAMES_COUNT && found < max_count; i++) {
        if (KNOWN_GAMES[i].steam_app_id == steam_app_id) {
            workarounds[found++] = &KNOWN_GAMES[i];
        }
    }
    
    return found;
}

int proton_game_db_apply_workaround(const game_workaround_t *workaround) {
    if (!workaround) {
        return -1;
    }
    
    // Apply environment variable overrides
    if (workaround->env_override) {
        char *env_copy = strdup(workaround->env_override);
        if (!env_copy) {
            return -1;
        }
        
        // Parse semicolon-separated list of VAR=value pairs
        char *token = strtok(env_copy, ";");
        while (token) {
            char *equals = strchr(token, '=');
            if (equals) {
                *equals = '\0';
                const char *var = token;
                const char *value = equals + 1;
                setenv(var, value, 1);
            }
            token = strtok(NULL, ";");
        }
        
        free(env_copy);
    }
    
    return 0;
}

int proton_game_db_get_count(void) {
    return KNOWN_GAMES_COUNT;
}

const game_workaround_t* proton_game_db_get_by_index(int index) {
    if (index < 0 || index >= KNOWN_GAMES_COUNT) {
        return NULL;
    }
    return &KNOWN_GAMES[index];
}
