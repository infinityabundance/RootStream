/*
 * hr_manager.c — Plugin hot-reload manager implementation
 *
 * The manager maintains a flat array of hr_entry_t.  dlopen/dlclose
 * are abstracted through function pointers so unit tests can inject
 * stubs without actual shared libraries.
 */

#include "hr_manager.h"

#include <stdlib.h>
#include <string.h>

/* ── fallback stubs (used when no real dl is available / testing) ── */
static void *stub_dlopen(const char *path, int flags) {
    (void)flags;
    /* Return non-NULL for any non-NULL path as a stub "handle" */
    return path ? (void *)(uintptr_t)0xDEAD : NULL;
}
static int stub_dlclose(void *handle) { (void)handle; return 0; }

struct hr_manager_s {
    hr_entry_t    entries[HR_MAX_PLUGINS];
    bool          used[HR_MAX_PLUGINS];
    int           count;
    hr_dlopen_fn  dl_open;
    hr_dlclose_fn dl_close;
};

hr_manager_t *hr_manager_create(hr_dlopen_fn  dlopen_fn,
                                  hr_dlclose_fn dlclose_fn) {
    hr_manager_t *mgr = calloc(1, sizeof(*mgr));
    if (!mgr) return NULL;
    mgr->dl_open  = dlopen_fn  ? dlopen_fn  : stub_dlopen;
    mgr->dl_close = dlclose_fn ? dlclose_fn : stub_dlclose;
    return mgr;
}

void hr_manager_destroy(hr_manager_t *mgr) { free(mgr); }

int hr_manager_plugin_count(const hr_manager_t *mgr) {
    return mgr ? mgr->count : 0;
}

static int find_slot(const hr_manager_t *mgr, const char *path) {
    for (int i = 0; i < HR_MAX_PLUGINS; i++)
        if (mgr->used[i] && strncmp(mgr->entries[i].path, path, HR_PATH_MAX) == 0)
            return i;
    return -1;
}

int hr_manager_register(hr_manager_t *mgr, const char *path) {
    if (!mgr || !path) return -1;
    if (mgr->count >= HR_MAX_PLUGINS) return -1;
    if (find_slot(mgr, path) >= 0) return -1; /* duplicate */
    for (int i = 0; i < HR_MAX_PLUGINS; i++) {
        if (!mgr->used[i]) {
            hr_entry_init(&mgr->entries[i], path);
            mgr->used[i] = true;
            mgr->count++;
            return 0;
        }
    }
    return -1;
}

int hr_manager_load(hr_manager_t *mgr, const char *path, uint64_t now_us) {
    if (!mgr || !path) return -1;
    int slot = find_slot(mgr, path);
    if (slot < 0) return -1;
    hr_entry_t *e = &mgr->entries[slot];

    void *h = mgr->dl_open(path, 0);
    if (!h) { e->state = HR_STATE_FAILED; return -1; }
    e->handle       = h;
    e->version++;
    e->state        = HR_STATE_LOADED;
    e->last_load_us = now_us;
    return 0;
}

int hr_manager_reload(hr_manager_t *mgr, const char *path, uint64_t now_us) {
    if (!mgr || !path) return -1;
    int slot = find_slot(mgr, path);
    if (slot < 0) return -1;
    hr_entry_t *e = &mgr->entries[slot];

    if (e->handle) { mgr->dl_close(e->handle); e->handle = NULL; }

    void *h = mgr->dl_open(path, 0);
    if (!h) { e->state = HR_STATE_FAILED; return -1; }
    e->handle       = h;
    e->version++;
    e->state        = HR_STATE_LOADED;
    e->last_load_us = now_us;
    return 0;
}

int hr_manager_unload(hr_manager_t *mgr, const char *path) {
    if (!mgr || !path) return -1;
    int slot = find_slot(mgr, path);
    if (slot < 0) return -1;
    hr_entry_t *e = &mgr->entries[slot];
    if (e->handle) { mgr->dl_close(e->handle); e->handle = NULL; }
    e->state = HR_STATE_UNLOADED;
    return 0;
}

const hr_entry_t *hr_manager_get(const hr_manager_t *mgr, const char *path) {
    if (!mgr || !path) return NULL;
    int slot = find_slot(mgr, path);
    return (slot >= 0) ? &mgr->entries[slot] : NULL;
}
