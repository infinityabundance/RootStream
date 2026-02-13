/**
 * @file proton_game_db.h
 * @brief Game-specific compatibility workarounds database
 * 
 * Maintains a database of known games and their required workarounds
 * for optimal streaming performance under Proton.
 */

#ifndef PROTON_GAME_DB_H
#define PROTON_GAME_DB_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Game workaround entry
 */
typedef struct {
    uint32_t steam_app_id;
    const char *game_name;
    const char *issue_description;
    const char *env_override;        // Environment variable overrides
    const char *dxvk_version_min;    // Minimum DXVK version required
    const char *vkd3d_version_min;   // Minimum VKD3D version required
    bool requires_async_compile;
    bool requires_low_latency_mode;
} game_workaround_t;

/**
 * Lookup game workarounds by Steam App ID
 * 
 * @param steam_app_id Steam App ID to lookup
 * @param workarounds Output array of pointers to workarounds
 * @param max_count Maximum number of workarounds to return
 * @return Number of workarounds found, or 0 if none
 */
int proton_game_db_lookup(uint32_t steam_app_id, 
                         const game_workaround_t **workarounds,
                         int max_count);

/**
 * Apply workarounds for a game
 * 
 * @param workaround Workaround to apply
 * @return 0 on success, -1 on failure
 */
int proton_game_db_apply_workaround(const game_workaround_t *workaround);

/**
 * Get all known games count
 * 
 * @return Number of games in database
 */
int proton_game_db_get_count(void);

/**
 * Get game by index
 * 
 * @param index Game index (0 to count-1)
 * @return Pointer to workaround, or NULL if invalid index
 */
const game_workaround_t* proton_game_db_get_by_index(int index);

#ifdef __cplusplus
}
#endif

#endif /* PROTON_GAME_DB_H */
