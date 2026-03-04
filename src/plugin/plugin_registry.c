/*
 * plugin_registry.c — Plugin discovery and registration implementation
 */

#include "plugin_registry.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#  include <windows.h>
#  define PLUGIN_EXT ".dll"
#else
#  include <dirent.h>
#  define PLUGIN_EXT ".so"
#endif

struct plugin_registry_s {
    plugin_handle_t      *plugins[PLUGIN_REGISTRY_MAX];
    size_t                count;
    plugin_host_api_t     host;   /* Copy: registry owns the table */
};

/* ── Helpers ──────────────────────────────────────────────────── */

static bool ends_with(const char *str, const char *suffix) {
    size_t slen = strlen(str);
    size_t suflen = strlen(suffix);
    if (slen < suflen) return false;
    return strcmp(str + slen - suflen, suffix) == 0;
}

/* ── Public API ───────────────────────────────────────────────── */

plugin_registry_t *plugin_registry_create(const plugin_host_api_t *host) {
    if (!host) return NULL;

    plugin_registry_t *reg = calloc(1, sizeof(*reg));
    if (!reg) return NULL;

    reg->host = *host;   /* shallow copy */
    return reg;
}

void plugin_registry_destroy(plugin_registry_t *registry) {
    if (!registry) return;

    for (size_t i = 0; i < registry->count; i++) {
        plugin_loader_unload(registry->plugins[i]);
        registry->plugins[i] = NULL;
    }
    free(registry);
}

int plugin_registry_load(plugin_registry_t *registry, const char *path) {
    if (!registry || !path) return -1;

    if (registry->count >= PLUGIN_REGISTRY_MAX) {
        fprintf(stderr, "[plugin_registry] registry full (%d)\n",
                PLUGIN_REGISTRY_MAX);
        return -1;
    }

    plugin_handle_t *h = plugin_loader_load(path, &registry->host);
    if (!h) return -1;

    registry->plugins[registry->count++] = h;
    return 0;
}

int plugin_registry_scan_dir(plugin_registry_t *registry, const char *dir) {
    if (!registry || !dir) return 0;

    int loaded = 0;

#ifdef _WIN32
    char pattern[512];
    snprintf(pattern, sizeof(pattern), "%s\\*%s", dir, PLUGIN_EXT);

    WIN32_FIND_DATAA fd;
    HANDLE hf = FindFirstFileA(pattern, &fd);
    if (hf == INVALID_HANDLE_VALUE) return 0;

    do {
        char full[512];
        snprintf(full, sizeof(full), "%s\\%s", dir, fd.cFileName);
        if (plugin_registry_load(registry, full) == 0) loaded++;
    } while (FindNextFileA(hf, &fd));
    FindClose(hf);
#else
    DIR *dp = opendir(dir);
    if (!dp) return 0;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        if (!ends_with(de->d_name, PLUGIN_EXT)) continue;

        char full[512];
        snprintf(full, sizeof(full), "%s/%s", dir, de->d_name);
        if (plugin_registry_load(registry, full) == 0) loaded++;
    }
    closedir(dp);
#endif

    return loaded;
}

int plugin_registry_unload(plugin_registry_t *registry, const char *name) {
    if (!registry || !name) return -1;

    for (size_t i = 0; i < registry->count; i++) {
        const plugin_descriptor_t *d =
            plugin_loader_get_descriptor(registry->plugins[i]);
        if (d && strcmp(d->name, name) == 0) {
            plugin_loader_unload(registry->plugins[i]);
            /* Compact array */
            memmove(&registry->plugins[i], &registry->plugins[i + 1],
                    (registry->count - i - 1) * sizeof(plugin_handle_t *));
            registry->count--;
            return 0;
        }
    }
    return -1;
}

size_t plugin_registry_count(const plugin_registry_t *registry) {
    return registry ? registry->count : 0;
}

plugin_handle_t *plugin_registry_get(const plugin_registry_t *registry,
                                     size_t index) {
    if (!registry || index >= registry->count) return NULL;
    return registry->plugins[index];
}

plugin_handle_t *plugin_registry_find_by_name(
        const plugin_registry_t *registry, const char *name) {
    if (!registry || !name) return NULL;

    for (size_t i = 0; i < registry->count; i++) {
        const plugin_descriptor_t *d =
            plugin_loader_get_descriptor(registry->plugins[i]);
        if (d && strcmp(d->name, name) == 0) {
            return registry->plugins[i];
        }
    }
    return NULL;
}

plugin_handle_t *plugin_registry_find_by_type(
        const plugin_registry_t *registry, plugin_type_t type) {
    if (!registry) return NULL;

    for (size_t i = 0; i < registry->count; i++) {
        const plugin_descriptor_t *d =
            plugin_loader_get_descriptor(registry->plugins[i]);
        if (d && d->type == type) {
            return registry->plugins[i];
        }
    }
    return NULL;
}
