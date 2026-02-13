/**
 * @file proton_test_demo.c
 * @brief Simple demo program to test Proton detection
 * 
 * Compile:
 *   gcc -o proton_demo proton_test_demo.c proton_detector.c -I.
 * 
 * Run:
 *   ./proton_demo
 *   
 * Or test with mock environment:
 *   PROTON_VERSION=8.3 DXVK_VERSION=1.10.3 DXVK_HUD=fps ./proton_demo
 */

#include "proton_detector.h"
#include "proton_game_db.h"
#include "proton_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("RootStream Proton Renderer - Detection Demo\n");
    printf("============================================\n\n");
    
    // Test Proton detection
    proton_info_t info;
    if (proton_detect(&info)) {
        printf("✓ Proton environment detected!\n\n");
        
        // Print detailed info
        char buf[2048];
        proton_info_to_string(&info, buf, sizeof(buf));
        printf("%s\n", buf);
        
        // Check Steam App ID
        if (info.steam_app_id[0] != '\0') {
            printf("\nChecking game database for App ID %s...\n", info.steam_app_id);
            
            uint32_t app_id = (uint32_t)atoi(info.steam_app_id);
            const game_workaround_t *workarounds[10];
            int count = proton_game_db_lookup(app_id, workarounds, 10);
            
            if (count > 0) {
                printf("Found %d workaround(s):\n", count);
                for (int i = 0; i < count; i++) {
                    printf("  - %s: %s\n", 
                           workarounds[i]->game_name,
                           workarounds[i]->issue_description);
                    if (workarounds[i]->env_override) {
                        printf("    Recommended: %s\n", workarounds[i]->env_override);
                    }
                }
            } else {
                printf("No specific workarounds found for this game.\n");
            }
        }
    } else {
        printf("✗ Proton environment not detected.\n");
        printf("\nTo test detection, set these environment variables:\n");
        printf("  PROTON_VERSION=8.3\n");
        printf("  WINEPREFIX=/path/to/prefix\n");
        printf("  DXVK_VERSION=1.10.3 (optional)\n");
        printf("  VKD3D_VERSION=1.2 (optional)\n");
        printf("  SteamAppId=570 (optional, e.g., for Dota 2)\n");
    }
    
    printf("\n");
    
    // Show game database stats
    int game_count = proton_game_db_get_count();
    printf("Game Database: %d known games with workarounds\n", game_count);
    
    // Show first few games
    printf("\nSample games in database:\n");
    for (int i = 0; i < game_count && i < 5; i++) {
        const game_workaround_t *game = proton_game_db_get_by_index(i);
        if (game) {
            printf("  %u - %s\n", game->steam_app_id, game->game_name);
        }
    }
    
    printf("\n");
    
    // Test settings
    proton_settings_t settings;
    proton_settings_get_default(&settings);
    printf("Default Settings:\n");
    printf("  DXVK: %s\n", settings.enable_dxvk ? "enabled" : "disabled");
    printf("  VKD3D: %s\n", settings.enable_vkd3d ? "enabled" : "disabled");
    printf("  Async Shader Compile: %s\n", 
           settings.enable_async_shader_compile ? "enabled" : "disabled");
    printf("  Shader Cache Max: %d MB\n", settings.shader_cache_max_mb);
    printf("  Preferred DirectX: %s\n", settings.preferred_directx_version);
    
    return 0;
}
