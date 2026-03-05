/*
 * hr_entry.h — Plugin hot-reload entry descriptor
 *
 * Represents a single dynamically-loaded plugin.  The entry stores the
 * path, a dlopen handle (void*), an integer version that is bumped on
 * each successful reload, and a state enum.
 *
 * Thread-safety: value type — no shared state.
 */

#ifndef ROOTSTREAM_HR_ENTRY_H
#define ROOTSTREAM_HR_ENTRY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HR_PATH_MAX  256   /**< Maximum plugin path length (incl. NUL) */

/** Plugin state */
typedef enum {
    HR_STATE_UNLOADED  = 0,
    HR_STATE_LOADED    = 1,
    HR_STATE_FAILED    = 2,
} hr_state_t;

/** Hot-reload plugin entry */
typedef struct {
    char      path[HR_PATH_MAX];  /**< Shared library path */
    void     *handle;             /**< dlopen handle (NULL if not loaded) */
    uint32_t  version;            /**< Reload counter (0 = never loaded) */
    hr_state_t state;
    uint64_t  last_load_us;       /**< Timestamp of last successful load (µs) */
} hr_entry_t;

/**
 * hr_entry_init — initialise entry
 *
 * @param e     Entry to initialise
 * @param path  Shared library path
 * @return      0 on success, -1 on NULL
 */
int hr_entry_init(hr_entry_t *e, const char *path);

/**
 * hr_entry_clear — reset entry to UNLOADED state
 *
 * @param e  Entry to clear
 */
void hr_entry_clear(hr_entry_t *e);

/**
 * hr_state_name — human-readable state name
 */
const char *hr_state_name(hr_state_t s);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_HR_ENTRY_H */
