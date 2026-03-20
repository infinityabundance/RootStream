/*
 * session_checkpoint.h — Checkpoint save/load for session persistence
 *
 * Writes a session_state_t snapshot to a file and reads it back.
 * A monotonic sequence number is embedded in the filename so the
 * most-recent checkpoint can be found quickly by listing the directory.
 *
 * File naming convention:
 *   <dir>/rootstream-ckpt-<session_id>-<seq>.bin
 *
 * Thread-safety: checkpoint_save and checkpoint_load are safe to call
 * from any thread (they perform atomic rename on write).
 */

#ifndef ROOTSTREAM_SESSION_CHECKPOINT_H
#define ROOTSTREAM_SESSION_CHECKPOINT_H

#include <stdbool.h>
#include <stdint.h>

#include "session_state.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CHECKPOINT_DIR_MAX 256 /**< Max directory path length */
#define CHECKPOINT_MAX_KEEP 3  /**< Keep this many old checkpoints */

/** Checkpoint manager configuration */
typedef struct {
    char dir[CHECKPOINT_DIR_MAX]; /**< Directory for checkpoint files */
    int max_keep;                 /**< Max old checkpoints to retain */
} checkpoint_config_t;

/** Opaque checkpoint manager */
typedef struct checkpoint_manager_s checkpoint_manager_t;

/**
 * checkpoint_manager_create — allocate checkpoint manager
 *
 * @param config  Configuration (NULL uses /tmp and max_keep=3)
 * @return        Non-NULL handle, or NULL on OOM
 */
checkpoint_manager_t *checkpoint_manager_create(const checkpoint_config_t *config);

/**
 * checkpoint_manager_destroy — free checkpoint manager
 *
 * Does not delete checkpoint files.
 *
 * @param mgr  Manager to destroy
 */
void checkpoint_manager_destroy(checkpoint_manager_t *mgr);

/**
 * checkpoint_save — write @state to a new checkpoint file
 *
 * Writes to a temp file then renames atomically.  If the number of
 * existing checkpoints for this session exceeds max_keep, the oldest
 * is deleted.
 *
 * @param mgr    Checkpoint manager
 * @param state  Session state to save
 * @return       0 on success, -1 on I/O error
 */
int checkpoint_save(checkpoint_manager_t *mgr, const session_state_t *state);

/**
 * checkpoint_load — load the most-recent checkpoint for @session_id
 *
 * Scans @mgr->dir for matching files and loads the highest sequence
 * number.
 *
 * @param mgr        Checkpoint manager
 * @param session_id Session to look up
 * @param state      Output session state
 * @return           0 on success, -1 if not found or corrupted
 */
int checkpoint_load(const checkpoint_manager_t *mgr, uint64_t session_id, session_state_t *state);

/**
 * checkpoint_delete — remove all checkpoint files for @session_id
 *
 * @param mgr        Checkpoint manager
 * @param session_id Session whose checkpoints to delete
 * @return           Number of files deleted (>= 0)
 */
int checkpoint_delete(checkpoint_manager_t *mgr, uint64_t session_id);

/**
 * checkpoint_exists — return true if at least one checkpoint file exists
 *
 * @param mgr        Checkpoint manager
 * @param session_id Session to check
 * @return           true if checkpoint exists
 */
bool checkpoint_exists(const checkpoint_manager_t *mgr, uint64_t session_id);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_SESSION_CHECKPOINT_H */
