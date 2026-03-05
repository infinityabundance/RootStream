/*
 * hr_manager.h — Plugin hot-reload manager
 *
 * Manages a registry of up to HR_MAX_PLUGINS hot-reloadable plugins.
 * On reload:
 *   1. dlclose the old handle (if loaded).
 *   2. dlopen the same path again.
 *   3. Increment the entry version on success.
 *   4. Set state to LOADED or FAILED.
 *
 * The manager uses a stub dlopen/dlclose implementation in tests (the
 * function pointers can be overridden for unit testing without actual
 * shared libraries).
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_HR_MANAGER_H
#define ROOTSTREAM_HR_MANAGER_H

#include "hr_entry.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HR_MAX_PLUGINS  16   /**< Maximum managed plugins */

/** dl function pointer types (overridable for testing) */
typedef void *(*hr_dlopen_fn)(const char *path, int flags);
typedef int   (*hr_dlclose_fn)(void *handle);

/** Opaque hot-reload manager */
typedef struct hr_manager_s hr_manager_t;

/**
 * hr_manager_create — allocate manager
 *
 * @param dlopen_fn   Override for dlopen  (NULL → use system dlopen)
 * @param dlclose_fn  Override for dlclose (NULL → use system dlclose)
 * @return            Non-NULL handle, or NULL on OOM
 */
hr_manager_t *hr_manager_create(hr_dlopen_fn  dlopen_fn,
                                  hr_dlclose_fn dlclose_fn);

/**
 * hr_manager_destroy — free manager (does NOT dlclose loaded plugins)
 */
void hr_manager_destroy(hr_manager_t *mgr);

/**
 * hr_manager_register — add a plugin path to the registry
 *
 * @param mgr   Manager
 * @param path  Shared library path
 * @return      0 on success, -1 if full or duplicate
 */
int hr_manager_register(hr_manager_t *mgr, const char *path);

/**
 * hr_manager_load — dlopen a registered plugin (first load)
 *
 * @param mgr   Manager
 * @param path  Plugin path
 * @param now_us Current time in µs (stored as last_load_us)
 * @return      0 on success, -1 on error or not found
 */
int hr_manager_load(hr_manager_t *mgr, const char *path, uint64_t now_us);

/**
 * hr_manager_reload — dlclose + dlopen a registered plugin
 *
 * @param mgr    Manager
 * @param path   Plugin path
 * @param now_us Current time in µs
 * @return       0 on success, -1 on failure (state set to FAILED)
 */
int hr_manager_reload(hr_manager_t *mgr, const char *path, uint64_t now_us);

/**
 * hr_manager_unload — dlclose a registered plugin
 *
 * @param mgr   Manager
 * @param path  Plugin path
 * @return      0 on success, -1 on error
 */
int hr_manager_unload(hr_manager_t *mgr, const char *path);

/**
 * hr_manager_get — get entry by path
 *
 * @param mgr   Manager
 * @param path  Plugin path
 * @return      Pointer to entry, or NULL if not found
 */
const hr_entry_t *hr_manager_get(const hr_manager_t *mgr, const char *path);

/**
 * hr_manager_plugin_count — number of registered plugins
 */
int hr_manager_plugin_count(const hr_manager_t *mgr);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_HR_MANAGER_H */
