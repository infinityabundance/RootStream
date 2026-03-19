/*
 * plugin_loader.h — Dynamic plugin loader
 *
 * Loads/unloads RootStream plugin shared objects using dlopen/dlclose
 * (POSIX) or LoadLibrary/FreeLibrary (Win32).
 */

#ifndef ROOTSTREAM_PLUGIN_LOADER_H
#define ROOTSTREAM_PLUGIN_LOADER_H

#include <stddef.h>

#include "plugin_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque handle for a loaded plugin */
typedef struct plugin_handle_s plugin_handle_t;

/**
 * plugin_loader_load — load a single plugin shared object
 *
 * Opens the shared object at @path, validates the magic/version, and
 * calls rs_plugin_init() with @host.
 *
 * @param path  Filesystem path to the .so / .dll
 * @param host  Host API table to pass to the plugin
 * @return      Non-NULL handle on success, NULL on failure
 */
plugin_handle_t *plugin_loader_load(const char *path, const plugin_host_api_t *host);

/**
 * plugin_loader_unload — shutdown and unload a plugin
 *
 * Calls rs_plugin_shutdown(), then dlclose().
 *
 * @param handle  Handle returned by plugin_loader_load()
 */
void plugin_loader_unload(plugin_handle_t *handle);

/**
 * plugin_loader_get_descriptor — return descriptor of a loaded plugin
 *
 * @param handle  Loaded plugin handle
 * @return        Pointer to the descriptor (lifetime = plugin lifetime)
 */
const plugin_descriptor_t *plugin_loader_get_descriptor(const plugin_handle_t *handle);

/**
 * plugin_loader_get_path — return the path used to load the plugin
 *
 * @param handle  Loaded plugin handle
 * @return        NUL-terminated path string (lifetime = handle lifetime)
 */
const char *plugin_loader_get_path(const plugin_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PLUGIN_LOADER_H */
