/*
 * tray_tui.c - ncurses text-based UI
 * 
 * Fallback when GTK unavailable (SSH, headless, etc).
 * Provides status, peer list, statistics in terminal.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#ifdef HAVE_NCURSES
#include <ncurses.h>

typedef struct {
    WINDOW *win;
    bool running;
} tui_ctx_t;

static void handle_resize(int sig) {
    (void)sig;
    /* Redraw on resize */
}

int tray_init_tui(rootstream_ctx_t *ctx, int argc, char **argv) {
    (void)argc;
    (void)argv;

    tui_ctx_t *tui = calloc(1, sizeof(tui_ctx_t));
    if (!tui) return -1;

    /* Initialize ncurses */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);  /* Non-blocking input */

    tui->win = stdscr;
    tui->running = true;
    ctx->tray_priv = tui;

    signal(SIGWINCH, handle_resize);

    printf("✓ Terminal UI initialized\n");
    return 0;
}

void tray_update_status_tui(rootstream_ctx_t *ctx, tray_status_t status) {
    if (!ctx || !ctx->tray_priv) return;

    clear();
    
    int row = 0;
    mvprintw(row++, 0, "╔════════════════════════════════════════╗");
    mvprintw(row++, 0, "║       RootStream Status               ║");
    mvprintw(row++, 0, "╚════════════════════════════════════════╝");
    row++;

    const char *status_str = "UNKNOWN";
    switch (status) {
        case STATUS_IDLE: status_str = "IDLE"; break;
        case STATUS_HOSTING: status_str = "HOSTING"; break;
        case STATUS_CONNECTED: status_str = "CONNECTED"; break;
        case STATUS_ERROR: status_str = "ERROR"; break;
    }

    mvprintw(row++, 0, "Status:  %s", status_str);
    mvprintw(row++, 0, "Peers:   %d connected", ctx->num_peers);
    
    row++;
    mvprintw(row++, 0, "Connected Peers:");
    for (int i = 0; i < ctx->num_peers && row < LINES - 5; i++) {
        mvprintw(row++, 2, "• %s (%s)", ctx->peers[i].hostname,
                ctx->peers[i].state == PEER_CONNECTED ? "connected" : "disconnected");
    }

    row++;
    mvprintw(row++, 0, "Statistics:");
    mvprintw(row++, 2, "Frames sent: %lu", ctx->frames_captured);
    mvprintw(row++, 2, "Bytes sent: %lu", ctx->bytes_sent);
    mvprintw(row++, 2, "Bytes received: %lu", ctx->bytes_received);

    row++;
    mvprintw(row++, 0, "Keys: [q]uit [l]ist peers [r]efresh");

    refresh();
}

void tray_show_qr_code_tui(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->tray_priv) return;
    
    clear();
    mvprintw(0, 0, "Your RootStream Code:");
    mvprintw(1, 0, "%s", ctx->keypair.rootstream_code);
    mvprintw(3, 0, "Share this code with peers to connect.");
    mvprintw(4, 0, "Press any key to continue...");
    refresh();
    
    /* Wait for keypress */
    nodelay(stdscr, FALSE);
    getch();
    nodelay(stdscr, TRUE);
}

void tray_show_peers_tui(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->tray_priv) return;

    clear();
    mvprintw(0, 0, "Connected Peers (%d):", ctx->num_peers);
    
    for (int i = 0; i < ctx->num_peers && i < LINES - 5; i++) {
        mvprintw(i + 2, 2, "%d. %s (%s:%u) - %s",
                i + 1, ctx->peers[i].hostname,
                ctx->peers[i].hostname, ctx->port,
                ctx->peers[i].state == PEER_CONNECTED ? "online" : "offline");
    }

    mvprintw(LINES - 2, 0, "Press any key to continue...");
    refresh();
    
    /* Wait for keypress */
    nodelay(stdscr, FALSE);
    getch();
    nodelay(stdscr, TRUE);
}

void tray_run_tui(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->tray_priv) return;
    
    /* Non-blocking check for user input */
    int ch = getch();
    if (ch == 'q' || ch == 'Q') {
        ctx->running = false;
    } else if (ch == 'l' || ch == 'L') {
        tray_show_peers_tui(ctx);
    } else if (ch == 'r' || ch == 'R') {
        tray_update_status_tui(ctx, ctx->tray.status);
    }
}

void tray_cleanup_tui(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->tray_priv) return;
    endwin();
    free(ctx->tray_priv);
    ctx->tray_priv = NULL;
}

#else

/* Stubs when ncurses not available */
int tray_init_tui(rootstream_ctx_t *ctx, int argc, char **argv) {
    (void)ctx;
    (void)argc;
    (void)argv;
    return -1;
}

void tray_update_status_tui(rootstream_ctx_t *ctx, tray_status_t status) {
    (void)ctx;
    (void)status;
}

void tray_show_qr_code_tui(rootstream_ctx_t *ctx) {
    (void)ctx;
}

void tray_show_peers_tui(rootstream_ctx_t *ctx) {
    (void)ctx;
}

void tray_run_tui(rootstream_ctx_t *ctx) {
    (void)ctx;
}

void tray_cleanup_tui(rootstream_ctx_t *ctx) {
    (void)ctx;
}

#endif
