/*
 * config.c - Configuration file management
 * 
 * Configuration directory: ~/.config/rootstream/
 * Files:
 *   - identity.pub      Ed25519 public key (32 bytes)
 *   - identity.key      Ed25519 private key (32 bytes, mode 0600)
 *   - identity.txt      Hostname/device name
 *   - config.ini        User preferences (TODO)
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
    return crypto_save_keypair(&ctx->keypair, config_dir);
}
