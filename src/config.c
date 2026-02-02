/*
 * config.c - Configuration file management
 *
 * Configuration directory: ~/.config/rootstream/
 * Files:
 *   - identity.pub      Ed25519 public key (32 bytes)
 *   - identity.key      Ed25519 private key (32 bytes, mode 0600)
 *   - identity.txt      Hostname/device name
 *   - config.ini        User preferences
 *
 * XDG Base Directory Specification compliance:
 * - Use $XDG_CONFIG_HOME if set, otherwise ~/.config
 * - Use $XDG_DATA_HOME for cache/logs if needed
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>

/*
 * Get configuration directory path
 *
 * @return Configuration directory path (never NULL)
 *
 * Priority:
 * 1. $XDG_CONFIG_HOME/rootstream
 * 2. $HOME/.config/rootstream
 * 3. /tmp/rootstream (fallback if no home)
 */
const char* config_get_dir(void) {
    static char config_dir[512] = {0};

    if (config_dir[0] != '\0') {
        return config_dir;  /* Already computed */
    }

    /* Try XDG_CONFIG_HOME */
    const char *xdg_config = getenv("XDG_CONFIG_HOME");
    if (xdg_config && xdg_config[0] != '\0') {
        snprintf(config_dir, sizeof(config_dir), "%s/rootstream", xdg_config);
        return config_dir;
    }

    /* Try HOME/.config */
    const char *home = getenv("HOME");
    if (!home || home[0] == '\0') {
        /* Try to get home from passwd */
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            home = pw->pw_dir;
        }
    }

    if (home && home[0] != '\0') {
        snprintf(config_dir, sizeof(config_dir), "%s/.config/rootstream", home);
        return config_dir;
    }

    /* Fallback to /tmp (not ideal, but works) */
    snprintf(config_dir, sizeof(config_dir), "/tmp/rootstream-%d", getuid());
    fprintf(stderr, "WARNING: Using fallback config directory: %s\n", config_dir);

    return config_dir;
}

/*
 * Initialize default settings
 */
static void config_init_defaults(settings_t *settings) {
    /* Video defaults */
    settings->video_bitrate = 10000000;  /* 10 Mbps */
    settings->video_framerate = 60;      /* 60 fps */
    strncpy(settings->video_codec, "h264", sizeof(settings->video_codec) - 1);

    /* Audio defaults */
    settings->audio_enabled = true;
    settings->audio_bitrate = 64000;     /* 64 kbps */

    /* Network defaults */
    settings->network_port = 9876;
    settings->discovery_enabled = true;

    /* Connection history */
    settings->peer_history_count = 0;
    settings->last_connected[0] = '\0';
}

/*
 * Trim whitespace from string
 */
static void trim(char *str) {
    if (!str) return;

    /* Trim leading */
    char *start = str;
    while (*start && (*start == ' ' || *start == '\t')) {
        start++;
    }

    /* Trim trailing */
    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    *(end + 1) = '\0';

    /* Move trimmed string to beginning */
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

/*
 * Load settings from config.ini
 *
 * @param settings Settings structure to populate
 * @param config_dir Configuration directory path
 * @return 0 on success, -1 on error
 */
static int config_load_ini(settings_t *settings, const char *config_dir) {
    char ini_path[512];
    int len = snprintf(ini_path, sizeof(ini_path), "%s/config.ini", config_dir);
    if (len < 0 || (size_t)len >= sizeof(ini_path)) {
        return -1;  /* Path too long */
    }

    FILE *fp = fopen(ini_path, "r");
    if (!fp) {
        /* No config file yet, use defaults */
        config_init_defaults(settings);
        return 0;
    }

    /* Initialize with defaults first */
    config_init_defaults(settings);

    char line[256];
    char section[64] = "";

    while (fgets(line, sizeof(line), fp)) {
        trim(line);

        /* Skip empty lines and comments */
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';') {
            continue;
        }

        /* Parse section header */
        if (line[0] == '[') {
            char *end = strchr(line, ']');
            if (end) {
                size_t section_len = (size_t)(end - (line + 1));
                if (section_len >= sizeof(section)) {
                    section_len = sizeof(section) - 1;
                }
                memcpy(section, line + 1, section_len);
                section[section_len] = '\0';
            }
            continue;
        }

        /* Parse key=value */
        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = line;
        char *value = eq + 1;
        trim(key);
        trim(value);

        /* Video settings */
        if (strcmp(section, "video") == 0) {
            if (strcmp(key, "bitrate") == 0) {
                settings->video_bitrate = (uint32_t)atoi(value);
            } else if (strcmp(key, "framerate") == 0) {
                settings->video_framerate = (uint32_t)atoi(value);
            } else if (strcmp(key, "codec") == 0) {
                strncpy(settings->video_codec, value, sizeof(settings->video_codec) - 1);
            }
        }
        /* Audio settings */
        else if (strcmp(section, "audio") == 0) {
            if (strcmp(key, "enabled") == 0) {
                settings->audio_enabled = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            } else if (strcmp(key, "bitrate") == 0) {
                settings->audio_bitrate = (uint32_t)atoi(value);
            }
        }
        /* Network settings */
        else if (strcmp(section, "network") == 0) {
            if (strcmp(key, "port") == 0) {
                settings->network_port = (uint16_t)atoi(value);
            } else if (strcmp(key, "discovery") == 0) {
                settings->discovery_enabled = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            }
        }
        /* Peer history */
        else if (strcmp(section, "peers") == 0) {
            if (strcmp(key, "last_connected") == 0) {
                strncpy(settings->last_connected, value, sizeof(settings->last_connected) - 1);
            } else if (strncmp(key, "peer_", 5) == 0) {
                int idx = atoi(key + 5);
                if (idx >= 0 && idx < MAX_PEER_HISTORY) {
                    strncpy(settings->peer_history[idx], value, ROOTSTREAM_CODE_MAX_LEN - 1);
                    if (idx >= settings->peer_history_count) {
                        settings->peer_history_count = idx + 1;
                    }
                }
            }
        }
    }

    fclose(fp);
    return 0;
}

/*
 * Save settings to config.ini
 *
 * @param settings Settings structure to save
 * @param config_dir Configuration directory path
 * @return 0 on success, -1 on error
 */
static int config_save_ini(const settings_t *settings, const char *config_dir) {
    char ini_path[512];
    snprintf(ini_path, sizeof(ini_path), "%s/config.ini", config_dir);

    /* Ensure directory exists */
    mkdir(config_dir, 0700);

    FILE *fp = fopen(ini_path, "w");
    if (!fp) {
        fprintf(stderr, "ERROR: Failed to open %s for writing\n", ini_path);
        return -1;
    }

    fprintf(fp, "# RootStream Configuration\n");
    fprintf(fp, "# Generated automatically - edit with caution\n\n");

    /* Video settings */
    fprintf(fp, "[video]\n");
    fprintf(fp, "bitrate = %u\n", settings->video_bitrate);
    fprintf(fp, "framerate = %u\n", settings->video_framerate);
    fprintf(fp, "codec = %s\n\n", settings->video_codec);

    /* Audio settings */
    fprintf(fp, "[audio]\n");
    fprintf(fp, "enabled = %s\n", settings->audio_enabled ? "true" : "false");
    fprintf(fp, "bitrate = %u\n\n", settings->audio_bitrate);

    /* Network settings */
    fprintf(fp, "[network]\n");
    fprintf(fp, "port = %u\n", settings->network_port);
    fprintf(fp, "discovery = %s\n\n", settings->discovery_enabled ? "true" : "false");

    /* Peer history */
    fprintf(fp, "[peers]\n");
    if (settings->last_connected[0] != '\0') {
        fprintf(fp, "last_connected = %s\n", settings->last_connected);
    }
    for (int i = 0; i < settings->peer_history_count; i++) {
        if (settings->peer_history[i][0] != '\0') {
            fprintf(fp, "peer_%d = %s\n", i, settings->peer_history[i]);
        }
    }

    fclose(fp);
    return 0;
}

/*
 * Load configuration and initialize identity
 *
 * @param ctx RootStream context
 * @return    0 on success, -1 on error
 *
 * If no keypair exists, generates a new one.
 * If keypair exists, loads it from disk.
 */
int config_load(rootstream_ctx_t *ctx) {
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context\n");
        return -1;
    }

    const char *config_dir = config_get_dir();

    /* Load settings from config.ini (or use defaults) */
    config_load_ini(&ctx->settings, config_dir);

    /* Try to load existing keypair */
    if (crypto_load_keypair(&ctx->keypair, config_dir) == 0) {
        /* Keypair loaded successfully */
        return 0;
    }

    /* No existing keypair, generate new one */
    printf("INFO: No existing keypair found\n");
    printf("INFO: Generating new identity...\n");

    /* Get hostname */
    char hostname[128];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        strcpy(hostname, "rootstream-device");
    }

    /* Generate keypair */
    if (crypto_generate_keypair(&ctx->keypair, hostname) < 0) {
        return -1;
    }

    /* Save to disk */
    if (crypto_save_keypair(&ctx->keypair, config_dir) < 0) {
        fprintf(stderr, "WARNING: Failed to save keypair\n");
        /* Non-fatal, can continue with in-memory keypair */
    }

    return 0;
}

/*
 * Add peer to connection history
 *
 * @param ctx RootStream context
 * @param rootstream_code Peer's rootstream code
 */
void config_add_peer_to_history(rootstream_ctx_t *ctx, const char *rootstream_code) {
    if (!ctx || !rootstream_code || rootstream_code[0] == '\0') {
        return;
    }

    /* Check if already in history */
    for (int i = 0; i < ctx->settings.peer_history_count; i++) {
        if (strcmp(ctx->settings.peer_history[i], rootstream_code) == 0) {
            /* Already in history, move to front */
            if (i > 0) {
                char temp[ROOTSTREAM_CODE_MAX_LEN];
                snprintf(temp, sizeof(temp), "%s", ctx->settings.peer_history[i]);
                /* Shift others down */
                for (int j = i; j > 0; j--) {
                    snprintf(ctx->settings.peer_history[j], ROOTSTREAM_CODE_MAX_LEN,
                            "%s", ctx->settings.peer_history[j-1]);
                }
                /* Put at front */
                snprintf(ctx->settings.peer_history[0], ROOTSTREAM_CODE_MAX_LEN, "%s", temp);
            }
            /* Update last connected */
            snprintf(ctx->settings.last_connected, sizeof(ctx->settings.last_connected),
                    "%s", rootstream_code);
            config_save(ctx);
            return;
        }
    }

    /* Not in history, add to front */
    if (ctx->settings.peer_history_count < MAX_PEER_HISTORY) {
        /* Shift all down */
        for (int i = ctx->settings.peer_history_count; i > 0; i--) {
            snprintf(ctx->settings.peer_history[i], ROOTSTREAM_CODE_MAX_LEN,
                    "%s", ctx->settings.peer_history[i-1]);
        }
        ctx->settings.peer_history_count++;
    } else {
        /* At max capacity, shift all down (dropping last) */
        for (int i = MAX_PEER_HISTORY - 1; i > 0; i--) {
            snprintf(ctx->settings.peer_history[i], ROOTSTREAM_CODE_MAX_LEN,
                    "%s", ctx->settings.peer_history[i-1]);
        }
    }

    /* Add new peer at front */
    snprintf(ctx->settings.peer_history[0], ROOTSTREAM_CODE_MAX_LEN, "%s", rootstream_code);

    /* Update last connected */
    snprintf(ctx->settings.last_connected, sizeof(ctx->settings.last_connected),
            "%s", rootstream_code);

    /* Save to disk */
    config_save(ctx);
}

/*
 * Save current configuration to disk
 *
 * @param ctx RootStream context
 * @return    0 on success, -1 on error
 */
int config_save(rootstream_ctx_t *ctx) {
    if (!ctx) {
        return -1;
    }

    const char *config_dir = config_get_dir();

    /* Save keypair */
    if (crypto_save_keypair(&ctx->keypair, config_dir) < 0) {
        return -1;
    }

    /* Save settings */
    return config_save_ini(&ctx->settings, config_dir);
}
