/*
 * plugin_api.h — RootStream Plugin ABI
 *
 * Defines the stable binary interface that all RootStream plugins must
 * implement.  Plugins are shared objects (.so / .dll) loaded at runtime
 * via the plugin_loader module.
 *
 * Plugin lifecycle
 * ────────────────
 *   1. plugin_loader discovers .so files in the plugin search path
 *   2. dlopen() loads the shared object
 *   3. plugin_loader calls rs_plugin_query() to get the descriptor
 *   4. plugin_loader calls rs_plugin_init() with the host API
 *   5. Plugin registers handlers and returns 0
 *   6. On shutdown plugin_loader calls rs_plugin_shutdown()
 *   7. dlclose() unloads the shared object
 *
 * ABI versioning
 * ──────────────
 *   PLUGIN_API_VERSION is incremented for every incompatible change.
 *   A plugin compiled against version N may only be loaded when the
 *   host's PLUGIN_API_VERSION == N.
 */

#ifndef ROOTSTREAM_PLUGIN_API_H
#define ROOTSTREAM_PLUGIN_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Version ─────────────────────────────────────────────────────── */

#define PLUGIN_API_VERSION 1
#define PLUGIN_API_MAGIC 0x52535054U /* 'RSPT' */

/* ── Plugin types ────────────────────────────────────────────────── */

typedef enum {
    PLUGIN_TYPE_UNKNOWN = 0,
    PLUGIN_TYPE_ENCODER = 1,   /**< Video/audio encoder backend */
    PLUGIN_TYPE_DECODER = 2,   /**< Video/audio decoder backend */
    PLUGIN_TYPE_CAPTURE = 3,   /**< Display/audio capture backend */
    PLUGIN_TYPE_FILTER = 4,    /**< Audio/video filter in pipeline */
    PLUGIN_TYPE_TRANSPORT = 5, /**< Network transport backend */
    PLUGIN_TYPE_UI = 6,        /**< UI extension (tray, overlay) */
} plugin_type_t;

/* ── Descriptor returned by rs_plugin_query() ────────────────────── */

typedef struct {
    uint32_t magic;        /**< Must equal PLUGIN_API_MAGIC */
    uint32_t api_version;  /**< Must equal PLUGIN_API_VERSION */
    plugin_type_t type;    /**< Plugin category */
    char name[64];         /**< Human-readable name, NUL-terminated */
    char version[16];      /**< Semver string e.g. "1.2.3" */
    char author[64];       /**< Author name or organisation */
    char description[256]; /**< One-line description */
} plugin_descriptor_t;

/* ── Host API provided to plugins ────────────────────────────────── */

/** Logging callback provided by the host */
typedef void (*plugin_log_fn_t)(const char *plugin_name, const char *level, const char *msg);

/** Host-side API table passed to rs_plugin_init() */
typedef struct {
    uint32_t api_version; /**< Must match PLUGIN_API_VERSION */
    plugin_log_fn_t log;  /**< Host logging function */
    void *host_ctx;       /**< Opaque host context */
    /* Reserved for future expansion */
    void *reserved[8];
} plugin_host_api_t;

/* ── Required plugin entry points ───────────────────────────────── */

/**
 * rs_plugin_query — return static descriptor (no allocation)
 *
 * Called immediately after dlopen().  Must not allocate or initialise
 * any resources; it is safe to call at any time.
 *
 * @return Pointer to a statically-allocated descriptor.
 */
typedef const plugin_descriptor_t *(*rs_plugin_query_fn_t)(void);

/**
 * rs_plugin_init — initialise the plugin
 *
 * @param host  Host API table; valid for the plugin's lifetime.
 * @return 0 on success, negative errno on failure.
 */
typedef int (*rs_plugin_init_fn_t)(const plugin_host_api_t *host);

/**
 * rs_plugin_shutdown — tear down the plugin
 *
 * Called before dlclose().  Must release all resources acquired in
 * rs_plugin_init().
 */
typedef void (*rs_plugin_shutdown_fn_t)(void);

/* Exported symbol names (used by dlsym) */
#define RS_PLUGIN_QUERY_SYMBOL "rs_plugin_query"
#define RS_PLUGIN_INIT_SYMBOL "rs_plugin_init"
#define RS_PLUGIN_SHUTDOWN_SYMBOL "rs_plugin_shutdown"

/* ── Convenience macro for plugin implementors ───────────────────── */

/**
 * RS_PLUGIN_DECLARE — emit the three required entry points
 *
 * Usage (in exactly one .c translation unit of the plugin):
 *
 *   static const plugin_descriptor_t MY_DESC = { ... };
 *   static int  my_init(const plugin_host_api_t *h) { ... }
 *   static void my_shutdown(void) { ... }
 *   RS_PLUGIN_DECLARE(MY_DESC, my_init, my_shutdown)
 */
#define RS_PLUGIN_DECLARE(desc_var, init_fn, shutdown_fn) \
    const plugin_descriptor_t *rs_plugin_query(void) {    \
        return &(desc_var);                               \
    }                                                     \
    int rs_plugin_init(const plugin_host_api_t *host) {   \
        return (init_fn)(host);                           \
    }                                                     \
    void rs_plugin_shutdown(void) {                       \
        (shutdown_fn)();                                  \
    }

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PLUGIN_API_H */
