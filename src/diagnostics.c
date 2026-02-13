/*
 * diagnostics.c - System capabilities and feature report
 * 
 * Prints detailed information about available backends and system capabilities
 * at startup. Useful for debugging and verification.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void diagnostics_print_header(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║              RootStream System Diagnostics Report             ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

void diagnostics_print_system_info(void) {
    printf("System Information:\n");
    printf("  Hostname: ");
    
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        printf("%s\n", hostname);
    } else {
        printf("(unknown)\n");
    }

    printf("  PID: %d\n", getpid());
    printf("  UID: %d (running as %s)\n", getuid(), getuid() == 0 ? "root" : "user");
    
    /* Check for render group (gid 44) */
    if (getuid() != 0) {
        printf("  GPU Group Access: ");
        
        /* Get list of supplementary groups */
        gid_t groups[64];
        int ngroups = sizeof(groups) / sizeof(groups[0]);
        if (getgroups(ngroups, groups) >= 0) {
            bool has_render_group = false;
            for (int i = 0; i < ngroups; i++) {
                if (groups[i] == 44) {  /* 44 = render group */
                    has_render_group = true;
                    break;
                }
            }
            printf("%s\n", has_render_group ? "YES (can use DRM)" : "NO (DRM may be limited)");
        } else {
            printf("(unknown)\n");
        }
    }
    printf("\n");
}

void diagnostics_print_display_info(void) {
    printf("Display Information:\n");
    
    const char *display = getenv("DISPLAY");
    printf("  DISPLAY: %s\n", display ? display : "(none - headless)");
    
    const char *wayland = getenv("WAYLAND_DISPLAY");
    printf("  WAYLAND: %s\n", wayland ? wayland : "(none)");
    
    printf("\n");
}

void diagnostics_print_available_backends(rootstream_ctx_t *ctx) {
    (void)ctx;
    printf("Available Backends:\n");
    
    printf("\n  Capture:\n");
    printf("    Primary (DRM/KMS):    %s\n", 
           access("/dev/dri/renderD128", F_OK) == 0 ? "✓ Available" : "✗ Not available");
#ifdef HAVE_X11
    printf("    Fallback 1 (X11):      ✓ Compiled in\n");
#else
    printf("    Fallback 1 (X11):      ✗ Not compiled\n");
#endif
    printf("    Fallback 2 (Dummy):    ✓ Always available\n");
    
    printf("\n  Encoder:\n");
    printf("    Primary (NVENC):       %s\n",
           access("/proc/driver/nvidia/gpus", F_OK) == 0 ? "✓ Available" : "✗ Not available");
    printf("    Primary (VA-API):      ");
#ifdef HAVE_VAAPI
    printf("✓ Compiled in\n");
#else
    printf("✗ Not compiled\n");
#endif
    printf("    Fallback (FFmpeg):     ");
#ifdef HAVE_FFMPEG
    printf("✓ Compiled in\n");
#else
    printf("✗ Not compiled\n");
#endif
    printf("    Fallback (Raw):        ✓ Always available\n");
    
    printf("\n  Audio Capture:\n");
    printf("    Primary (ALSA):        ✓ Compiled in\n");
    printf("    Fallback 1 (PulseAudio): ");
#ifdef HAVE_PULSEAUDIO
    printf("✓ Compiled in\n");
#else
    printf("✗ Not compiled\n");
#endif
    printf("    Fallback 2 (PipeWire):   ");
#ifdef HAVE_PIPEWIRE
    printf("✓ Compiled in\n");
#else
    printf("✗ Not compiled\n");
#endif
    printf("    Fallback 3 (Dummy):      ✓ Always available\n");
    
    printf("\n  Input Injection:\n");
    printf("    Primary (uinput):      %s\n",
           access("/dev/uinput", F_OK) == 0 ? "✓ Available" : "✗ Not available");
    /* Check for xdotool in common paths */
    bool xdotool_found = (access("/usr/bin/xdotool", X_OK) == 0 ||
                          access("/usr/local/bin/xdotool", X_OK) == 0 ||
                          access("/bin/xdotool", X_OK) == 0);
    printf("    Fallback (xdotool):    %s\n",
           xdotool_found ? "✓ Installed" : "✗ Not installed");
    printf("    Fallback (Logging):    ✓ Always available\n");
    
    printf("\n  GUI:\n");
#ifdef HAVE_GTK
    printf("    Primary (GTK Tray):    ✓ Compiled in\n");
#else
    printf("    Primary (GTK Tray):    ✗ Not compiled\n");
#endif
#ifdef HAVE_NCURSES
    printf("    Fallback (TUI):        ✓ Compiled in\n");
#else
    printf("    Fallback (TUI):        ✗ Not compiled\n");
#endif
    printf("    Fallback (CLI):        ✓ Always available\n");
    
    printf("\n  Discovery:\n");
#ifdef HAVE_AVAHI
    printf("    Primary (mDNS/Avahi):  ✓ Compiled in\n");
#else
    printf("    Primary (mDNS/Avahi):  ✗ Not compiled\n");
#endif
    printf("    Fallback (Broadcast):  ✓ Always available\n");
    printf("    Fallback (Manual):     ✓ Always available\n");
    
    printf("\n  Network:\n");
    printf("    Primary (UDP):         ✓ Always available\n");
    printf("    Fallback (TCP):        ✓ Always available\n");
    
    printf("\n");
}

void diagnostics_print_active_backends(rootstream_ctx_t *ctx) {
    printf("Active Backends (Runtime Selection):\n\n");
    
    printf("  Capture:       %s\n", ctx->active_backend.capture_name);
    printf("  Encoder:       %s\n", ctx->active_backend.encoder_name);
    printf("  Audio Capture: %s\n", ctx->active_backend.audio_cap_name ? 
           ctx->active_backend.audio_cap_name : "disabled");
    printf("  Audio Playback: %s\n", ctx->active_backend.audio_play_name ? 
           ctx->active_backend.audio_play_name : "disabled");
    printf("  Discovery:     %s\n", ctx->active_backend.discovery_name ? 
           ctx->active_backend.discovery_name : "uninitialized");
    printf("  Input:         %s\n", ctx->active_backend.input_name ? 
           ctx->active_backend.input_name : "uninitialized");
    printf("  GUI:           %s\n", ctx->active_backend.gui_name ? 
           ctx->active_backend.gui_name : "uninitialized");
    
    printf("\n");
}

void diagnostics_print_recommendations(rootstream_ctx_t *ctx) {
    (void)ctx;
    printf("Recommendations:\n");
    
    int recommendations = 0;
    
    if (access("/dev/uinput", F_OK) != 0) {
        printf("  • Install input support: sudo apt install xdotool\n");
        recommendations++;
    }
    
#ifndef HAVE_FFMPEG
    printf("  • Install software encoder: apt-get install libavcodec-dev libx264-dev\n");
    recommendations++;
#endif
    
#ifndef HAVE_PULSEAUDIO
    printf("  • Install PulseAudio support: apt-get install libpulse-dev\n");
    recommendations++;
#endif
    
    if (recommendations == 0) {
        printf("  ✓ System is fully configured!\n");
    }
    
    printf("\n");
}

/*
 * Print complete diagnostic report
 */
void diagnostics_print_report(rootstream_ctx_t *ctx) {
    diagnostics_print_header();
    diagnostics_print_system_info();
    diagnostics_print_display_info();
    diagnostics_print_available_backends(ctx);
    diagnostics_print_active_backends(ctx);
    diagnostics_print_recommendations(ctx);
}
