# RootStream Plugin API Guide

> Reference for third-party developers writing RootStream plugins.

---

## Overview

RootStream supports runtime-loadable plugins that extend or replace core
subsystems — encoders, decoders, capture backends, audio/video filters,
network transports, and UI extensions.

Plugins are ordinary shared objects (`.so` on Linux, `.dll` on Windows)
that export three C symbols:

| Symbol | Purpose |
|--------|---------|
| `rs_plugin_query` | Return static plugin descriptor (no allocation) |
| `rs_plugin_init` | Initialise plugin, register handlers with host |
| `rs_plugin_shutdown` | Release all resources before unload |

---

## Quick-start

```c
/* my_encoder_plugin.c */
#include <rootstream/plugin_api.h>
#include <string.h>

static const plugin_host_api_t *g_host;

static const plugin_descriptor_t MY_DESC = {
    .magic       = PLUGIN_API_MAGIC,
    .api_version = PLUGIN_API_VERSION,
    .type        = PLUGIN_TYPE_ENCODER,
    .name        = "my-h265-encoder",
    .version     = "1.0.0",
    .author      = "ACME Corp",
    .description = "H.265/HEVC software encoder via libx265",
};

static int my_init(const plugin_host_api_t *host) {
    g_host = host;
    host->log("my-h265-encoder", "info", "initialised");
    return 0;   /* 0 = success */
}

static void my_shutdown(void) {
    g_host = NULL;
}

RS_PLUGIN_DECLARE(MY_DESC, my_init, my_shutdown)
```

Build with:

```bash
gcc -shared -fPIC -o my_encoder_plugin.so my_encoder_plugin.c \
    -I/usr/include/rootstream
```

Install into the plugin directory:

```bash
sudo cp my_encoder_plugin.so /usr/lib/rootstream/plugins/
```

---

## Plugin Types

| `plugin_type_t` | Value | When to use |
|-----------------|-------|-------------|
| `PLUGIN_TYPE_ENCODER`   | 1 | Custom video/audio encoder |
| `PLUGIN_TYPE_DECODER`   | 2 | Custom video/audio decoder |
| `PLUGIN_TYPE_CAPTURE`   | 3 | Custom display or audio capture |
| `PLUGIN_TYPE_FILTER`    | 4 | DSP audio/video filter node |
| `PLUGIN_TYPE_TRANSPORT` | 5 | Custom network transport |
| `PLUGIN_TYPE_UI`        | 6 | Tray icon or overlay extension |

---

## ABI Versioning

`PLUGIN_API_VERSION` is incremented on every incompatible ABI change.
A plugin compiled against version *N* may only be loaded by a host whose
`PLUGIN_API_VERSION` equals *N*.  The host rejects mismatched plugins with
an error message.

Always re-compile plugins after upgrading RootStream.

---

## Host API

The `plugin_host_api_t` table is passed to `rs_plugin_init()` and remains
valid for the plugin's entire lifetime.

```c
typedef struct {
    uint32_t         api_version;   /* PLUGIN_API_VERSION */
    plugin_log_fn_t  log;           /* log(plugin_name, level, msg) */
    void            *host_ctx;      /* Opaque host context */
    void            *reserved[8];   /* Future expansion */
} plugin_host_api_t;
```

### Logging

Call `host->log(plugin_name, level, msg)` for structured log output:

```c
host->log("my-encoder", "warn", "frame dropped: buffer full");
```

Level strings: `"debug"`, `"info"`, `"warn"`, `"error"`.

---

## Plugin Search Path

RootStream searches for plugins in:

1. Directories listed in the `ROOTSTREAM_PLUGIN_PATH` environment variable
   (colon-separated).
2. `~/.local/lib/rootstream/plugins/` (per-user)
3. `/usr/local/lib/rootstream/plugins/`
4. `/usr/lib/rootstream/plugins/`

Override at runtime:

```bash
ROOTSTREAM_PLUGIN_PATH=/opt/my_plugins ./rootstream --service
```

---

## Listing Loaded Plugins

```bash
rootstream --list-plugins
```

Output example:

```
Loaded plugins (2):
  [ENCODER] my-h265-encoder v1.0.0  (ACME Corp)
  [FILTER]  noise-suppressor  v0.3.1 (OpenSource Inc)
```

---

## Thread Safety

- `rs_plugin_query()` may be called from any thread.
- `rs_plugin_init()` and `rs_plugin_shutdown()` are called from the main
  thread only.
- All other callbacks are invoked on the calling subsystem's thread;
  plugins must synchronise their own internal state.

---

## Example: Audio Filter Plugin

```c
#include <rootstream/plugin_api.h>
#include <string.h>
#include <math.h>

static const plugin_host_api_t *g_host;

static const plugin_descriptor_t NOISE_DESC = {
    .magic       = PLUGIN_API_MAGIC,
    .api_version = PLUGIN_API_VERSION,
    .type        = PLUGIN_TYPE_FILTER,
    .name        = "simple-noise-gate",
    .version     = "0.1.0",
    .author      = "Example",
    .description = "Silence audio frames below -40 dBFS",
};

#define GATE_THRESHOLD_LINEAR 0.01f   /* ≈ -40 dBFS */

/* Process 16-bit PCM samples in-place */
void noise_gate_process(int16_t *samples, size_t count) {
    float rms = 0.0f;
    for (size_t i = 0; i < count; i++) {
        float s = samples[i] / 32768.0f;
        rms += s * s;
    }
    rms = sqrtf(rms / (float)count);
    if (rms < GATE_THRESHOLD_LINEAR) {
        memset(samples, 0, count * sizeof(int16_t));
    }
}

static int noise_init(const plugin_host_api_t *host) {
    g_host = host;
    host->log("simple-noise-gate", "info", "noise gate ready");
    return 0;
}

static void noise_shutdown(void) {
    g_host = NULL;
}

RS_PLUGIN_DECLARE(NOISE_DESC, noise_init, noise_shutdown)
```

---

## Security Considerations

- Plugins run in the same process as RootStream with the same privileges.
  Only load plugins from trusted sources.
- There is no sandbox or permission model.  Future versions may add
  capability restrictions via `host_ctx`.

---

*See `src/plugin/plugin_api.h` for the canonical ABI definition.*
