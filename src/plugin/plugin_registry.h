/*
 * plugin_registry.h — Plugin discovery and registration
 *
 * Scans one or more directories for .so / .dll files, loads each via
 * plugin_loader, and provides a typed lookup table indexed by
 * plugin_type_t.
 */

#ifndef ROOTSTREAM_PLUGIN_REGISTRY_H
#define ROOTSTREAM_PLUGIN_REGISTRY_H

#include <stddef.h>

#include "plugin_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Default plugin search path (colon-separated, like $PATH) */
#define PLUGIN_DEFAULT_SEARCH_PATH "/usr/lib/rootstream/plugins:/usr/local/lib/rootstream/plugins"

/** Maximum number of simultaneously loaded plugins */
#define PLUGIN_REGISTRY_MAX 64

/** Opaque registry handle */
typedef struct plugin_registry_s plugin_registry_t;

/**
 * plugin_registry_create — allocate an empty registry
 *
 * @param host  Host API table forwarded to every loaded plugin
 * @return      Non-NULL registry handle, or NULL on OOM
 */
plugin_registry_t *plugin_registry_create(const plugin_host_api_t *host);

/**
 * plugin_registry_destroy — unload all plugins and free the registry
 *
 * @param registry  Registry to destroy
 */
void plugin_registry_destroy(plugin_registry_t *registry);

/**
 * plugin_registry_scan_dir — discover and load plugins in @dir
 *
 * Iterates directory entries with extension ".so" (or ".dll" on Windows),
 * attempts to load each as a RootStream plugin.  Non-plugin shared objects
 * are silently skipped.
 *
 * @param registry  Target registry
 * @param dir       Filesystem directory path
 * @return          Number of plugins successfully loaded (≥ 0)
 */
int plugin_registry_scan_dir(plugin_registry_t *registry, const char *dir);

/**
 * plugin_registry_load — explicitly load a single plugin
 *
 * @param registry  Target registry
 * @param path      Path to the .so / .dll file
 * @return          0 on success, -1 on failure
 */
int plugin_registry_load(plugin_registry_t *registry, const char *path);

/**
 * plugin_registry_unload — unload a plugin by name
 *
 * @param registry  Target registry
 * @param name      plugin_descriptor_t::name to match
 * @return          0 on success, -1 if not found
 */
int plugin_registry_unload(plugin_registry_t *registry, const char *name);

/**
 * plugin_registry_count — return total number of loaded plugins
 *
 * @param registry  Registry
 * @return          Count
 */
size_t plugin_registry_count(const plugin_registry_t *registry);

/**
 * plugin_registry_get — retrieve a loaded handle by index
 *
 * @param registry  Registry
 * @param index     0-based index (< plugin_registry_count())
 * @return          Plugin handle, or NULL if out of range
 */
plugin_handle_t *plugin_registry_get(const plugin_registry_t *registry, size_t index);

/**
 * plugin_registry_find_by_name — look up a plugin by descriptor name
 *
 * @param registry  Registry
 * @param name      Exact match of plugin_descriptor_t::name
 * @return          Plugin handle, or NULL if not found
 */
plugin_handle_t *plugin_registry_find_by_name(const plugin_registry_t *registry, const char *name);

/**
 * plugin_registry_find_by_type — return first plugin matching @type
 *
 * @param registry  Registry
 * @param type      Plugin category
 * @return          Plugin handle, or NULL if none registered for @type
 */
plugin_handle_t *plugin_registry_find_by_type(const plugin_registry_t *registry,
                                              plugin_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PLUGIN_REGISTRY_H */
