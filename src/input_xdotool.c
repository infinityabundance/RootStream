/*
 * input_xdotool.c - X11 input injection via xdotool
 * 
 * Fallback when uinput unavailable.
 * Uses xdotool external command (subprocess).
 * Works on X11 systems even without kernel uinput.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/input-event-codes.h>

typedef struct {
    bool available;
} xdotool_ctx_t;

/*
 * Check if xdotool is installed
 */
bool input_xdotool_available(void) {
    /* Try to run xdotool --version */
    int ret = system("which xdotool > /dev/null 2>&1");
    return ret == 0;
}

/*
 * Initialize xdotool input backend
 */
int input_init_xdotool(rootstream_ctx_t *ctx) {
    if (!input_xdotool_available()) {
        fprintf(stderr, "ERROR: xdotool not found (install: apt-get install xdotool)\n");
        return -1;
    }

    xdotool_ctx_t *xdt = calloc(1, sizeof(xdotool_ctx_t));
    if (!xdt) return -1;

    xdt->available = true;
    ctx->input_priv = xdt;
    printf("✓ xdotool input backend initialized\n");
    return 0;
}

/*
 * Inject keyboard event via xdotool
 * Note: This is a simplified mapping for demonstration
 */
int input_inject_key_xdotool(uint32_t keycode, bool press) {
    if (keycode == 0) return -1;

    /* Map a few common evdev keycodes to xdotool key names */
    const char *key_name = NULL;
    switch (keycode) {
        case KEY_A: key_name = "a"; break;
        case KEY_B: key_name = "b"; break;
        case KEY_C: key_name = "c"; break;
        case KEY_D: key_name = "d"; break;
        case KEY_E: key_name = "e"; break;
        case KEY_F: key_name = "f"; break;
        case KEY_G: key_name = "g"; break;
        case KEY_H: key_name = "h"; break;
        case KEY_I: key_name = "i"; break;
        case KEY_J: key_name = "j"; break;
        case KEY_K: key_name = "k"; break;
        case KEY_L: key_name = "l"; break;
        case KEY_M: key_name = "m"; break;
        case KEY_N: key_name = "n"; break;
        case KEY_O: key_name = "o"; break;
        case KEY_P: key_name = "p"; break;
        case KEY_Q: key_name = "q"; break;
        case KEY_R: key_name = "r"; break;
        case KEY_S: key_name = "s"; break;
        case KEY_T: key_name = "t"; break;
        case KEY_U: key_name = "u"; break;
        case KEY_V: key_name = "v"; break;
        case KEY_W: key_name = "w"; break;
        case KEY_X: key_name = "x"; break;
        case KEY_Y: key_name = "y"; break;
        case KEY_Z: key_name = "z"; break;
        case KEY_SPACE: key_name = "space"; break;
        case KEY_ENTER: key_name = "Return"; break;
        case KEY_ESC: key_name = "Escape"; break;
        case KEY_TAB: key_name = "Tab"; break;
        default: return -1;
    }

    if (!key_name) return -1;

    char cmd[256];
    if (press) {
        snprintf(cmd, sizeof(cmd), "xdotool keydown %s 2>/dev/null", key_name);
    } else {
        snprintf(cmd, sizeof(cmd), "xdotool keyup %s 2>/dev/null", key_name);
    }

    int ret = system(cmd);
    return ret == 0 ? 0 : -1;
}

/*
 * Inject mouse event via xdotool
 */
int input_inject_mouse_xdotool(int x, int y, uint32_t buttons) {
    char cmd[256];
    int ret;
    
    /* Move mouse */
    snprintf(cmd, sizeof(cmd), "xdotool mousemove %d %d 2>/dev/null", x, y);
    if (system(cmd) != 0) return -1;

    /* Handle button clicks */
    if (buttons & BTN_LEFT) {
        ret = system("xdotool click 1 2>/dev/null");
        (void)ret;  /* Ignore result */
    }
    if (buttons & BTN_MIDDLE) {
        ret = system("xdotool click 2 2>/dev/null");
        (void)ret;  /* Ignore result */
    }
    if (buttons & BTN_RIGHT) {
        ret = system("xdotool click 3 2>/dev/null");
        (void)ret;  /* Ignore result */
    }

    return 0;
}

/*
 * Cleanup xdotool backend
 */
void input_cleanup_xdotool(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->input_priv) return;
    free(ctx->input_priv);
    ctx->input_priv = NULL;
}
