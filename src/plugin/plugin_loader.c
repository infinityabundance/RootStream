/*
 * plugin_loader.c — Dynamic plugin loader implementation
 */

#include "plugin_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
typedef HMODULE dl_handle_t;
#define dl_open(p) LoadLibraryA(p)
#define dl_sym(h, s) ((void *)GetProcAddress((h), (s)))
#define dl_close(h) FreeLibrary(h)
#define dl_error() "LoadLibrary failed"
#else
#include <dlfcn.h>
typedef void *dl_handle_t;
#define dl_open(p) dlopen((p), RTLD_NOW | RTLD_LOCAL)
#define dl_sym(h, s) dlsym((h), (s))
#define dl_close(h) dlclose(h)
#define dl_error() dlerror()
#endif

struct plugin_handle_s {
    dl_handle_t dl;                        /* OS library handle */
    const plugin_descriptor_t *descriptor; /* From rs_plugin_query */
    rs_plugin_shutdown_fn_t shutdown_fn;   /* From rs_plugin_shutdown */
    char path[512];                        /* Resolved load path */
};

/* ── Internal helpers ──────────────────────────────────────────── */

static void *load_sym(dl_handle_t dl, const char *name) {
    void *sym = dl_sym(dl, name);
    if (!sym) {
        fprintf(stderr, "[plugin_loader] symbol '%s' not found: %s\n", name, dl_error());
    }
    return sym;
}

/* ── Public API ────────────────────────────────────────────────── */

plugin_handle_t *plugin_loader_load(const char *path, const plugin_host_api_t *host) {
    if (!path || !host) {
        return NULL;
    }

    dl_handle_t dl = dl_open(path);
    if (!dl) {
        fprintf(stderr, "[plugin_loader] dlopen('%s') failed: %s\n", path, dl_error());
        return NULL;
    }

    /* Resolve required entry points */
    rs_plugin_query_fn_t query_fn = (rs_plugin_query_fn_t)load_sym(dl, RS_PLUGIN_QUERY_SYMBOL);
    rs_plugin_init_fn_t init_fn = (rs_plugin_init_fn_t)load_sym(dl, RS_PLUGIN_INIT_SYMBOL);
    rs_plugin_shutdown_fn_t shutdown_fn =
        (rs_plugin_shutdown_fn_t)load_sym(dl, RS_PLUGIN_SHUTDOWN_SYMBOL);

    if (!query_fn || !init_fn || !shutdown_fn) {
        dl_close(dl);
        return NULL;
    }

    /* Validate descriptor */
    const plugin_descriptor_t *desc = query_fn();
    if (!desc) {
        fprintf(stderr, "[plugin_loader] rs_plugin_query returned NULL\n");
        dl_close(dl);
        return NULL;
    }

    if (desc->magic != PLUGIN_API_MAGIC) {
        fprintf(stderr, "[plugin_loader] bad magic 0x%08X in '%s'\n", desc->magic, path);
        dl_close(dl);
        return NULL;
    }

    if (desc->api_version != PLUGIN_API_VERSION) {
        fprintf(stderr, "[plugin_loader] API version mismatch: plugin=%u host=%u\n",
                desc->api_version, PLUGIN_API_VERSION);
        dl_close(dl);
        return NULL;
    }

    /* Initialise the plugin */
    int rc = init_fn(host);
    if (rc != 0) {
        fprintf(stderr, "[plugin_loader] rs_plugin_init failed: %d\n", rc);
        dl_close(dl);
        return NULL;
    }

    plugin_handle_t *handle = calloc(1, sizeof(*handle));
    if (!handle) {
        shutdown_fn();
        dl_close(dl);
        return NULL;
    }

    handle->dl = dl;
    handle->descriptor = desc;
    handle->shutdown_fn = shutdown_fn;
    snprintf(handle->path, sizeof(handle->path), "%s", path);

    fprintf(stderr, "[plugin_loader] loaded plugin '%s' v%s from '%s'\n", desc->name, desc->version,
            path);

    return handle;
}

void plugin_loader_unload(plugin_handle_t *handle) {
    if (!handle) {
        return;
    }

    fprintf(stderr, "[plugin_loader] unloading plugin '%s'\n",
            handle->descriptor ? handle->descriptor->name : "(unknown)");

    if (handle->shutdown_fn) {
        handle->shutdown_fn();
    }

    if (handle->dl) {
        dl_close(handle->dl);
    }

    free(handle);
}

const plugin_descriptor_t *plugin_loader_get_descriptor(const plugin_handle_t *handle) {
    return handle ? handle->descriptor : NULL;
}

const char *plugin_loader_get_path(const plugin_handle_t *handle) {
    return handle ? handle->path : NULL;
}
