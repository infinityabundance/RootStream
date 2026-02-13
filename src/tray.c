/*
 * tray.c - GTK3 system tray application
 * 
 * Features:
 * - System tray icon with status indicator
 * - Right-click menu:
 *   - Show My QR Code
 *   - Connect to Peer (paste code)
 *   - View Connected Peers
 *   - Settings
 *   - About
 *   - Quit
 * - QR code display window
 * - Peer list window
 * - Status notifications
 * 
 * Design:
 * - Uses GtkStatusIcon for tray (legacy but widely supported)
 * - Minimal dependencies (just GTK3)
 * - Clean, modern UI following GNOME HIG
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

/* Icon paths (will be installed to /usr/share/icons) */
#define ICON_IDLE       "rootstream-idle"
#define ICON_HOSTING    "rootstream-hosting"
#define ICON_CONNECTED  "rootstream-connected"
#define ICON_ERROR      "rootstream-error"

/* Global context for GTK callbacks */
static rootstream_ctx_t *g_ctx = NULL;

/*
 * Callback for copy button timeout (reset button text)
 */
static gboolean on_copy_timeout(gpointer btn) {
    gtk_button_set_label(GTK_BUTTON(btn), "Copy to Clipboard");
    return G_SOURCE_REMOVE;
}

/*
 * Callback for copy button click
 */
static void on_copy_btn_clicked(GtkButton *btn, gpointer data) {
    (void)data;
    GtkClipboard *clip = (GtkClipboard*)g_object_get_data(G_OBJECT(btn), "clipboard");
    const char *text = (const char*)g_object_get_data(G_OBJECT(btn), "text");
    gtk_clipboard_set_text(clip, text, -1);

    /* Show brief notification */
    gtk_button_set_label(GTK_BUTTON(btn), "✓ Copied!");
    g_timeout_add(2000, on_copy_timeout, btn);
}

/*
 * Show QR code window
 * 
 * Displays:
 * - Large QR code image
 * - RootStream code text (selectable)
 * - Copy button
 * - Instructions
 */
static void on_show_qr_code(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    rootstream_ctx_t *ctx = (rootstream_ctx_t*)user_data;

    if (!ctx) return;

    /* Create window */
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "My RootStream Code");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(window), 20);

    /* Main vertical box */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    /* Title label */
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), 
        "<span font='18' weight='bold'>Share This Code to Connect</span>");
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);

    /* Generate QR code image */
    char qr_path[256];
    snprintf(qr_path, sizeof(qr_path), "/tmp/rootstream-qr-%d.png", getpid());
    qrcode_generate(ctx->keypair.rootstream_code, qr_path);

    /* QR code image */
    GtkWidget *image = gtk_image_new_from_file(qr_path);
    gtk_box_pack_start(GTK_BOX(vbox), image, TRUE, TRUE, 0);

    /* RootStream code entry (selectable) */
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), ctx->keypair.rootstream_code);
    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
    gtk_entry_set_alignment(GTK_ENTRY(entry), 0.5);  /* Center text */
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

    /* Copy button */
    GtkWidget *copy_btn = gtk_button_new_with_label("Copy to Clipboard");
    gtk_box_pack_start(GTK_BOX(vbox), copy_btn, FALSE, FALSE, 0);

    /* Copy to clipboard handler */
    g_signal_connect_swapped(copy_btn, "clicked",
        G_CALLBACK(gtk_entry_grab_focus_without_selecting), entry);
    g_signal_connect_swapped(copy_btn, "clicked",
        G_CALLBACK(gtk_editable_select_region), entry);
    
    /* Actually copy on button click */
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    g_object_set_data(G_OBJECT(copy_btn), "clipboard", clipboard);
    g_object_set_data(G_OBJECT(copy_btn), "text",
                      g_strdup(ctx->keypair.rootstream_code));

    g_signal_connect(copy_btn, "clicked",
        G_CALLBACK(on_copy_btn_clicked), NULL);

    /* Instructions */
    GtkWidget *instructions = gtk_label_new(
        "Scan this QR code or share the code above\n"
        "with another RootStream device to connect.");
    gtk_label_set_justify(GTK_LABEL(instructions), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), instructions, FALSE, FALSE, 0);

    /* Close button */
    GtkWidget *close_btn = gtk_button_new_with_label("Close");
    g_signal_connect_swapped(close_btn, "clicked",
        G_CALLBACK(gtk_widget_destroy), window);
    gtk_box_pack_start(GTK_BOX(vbox), close_btn, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
}

/*
 * Connect to peer dialog
 * 
 * Prompts user to paste a RootStream code
 */
static void on_connect_to_peer(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    rootstream_ctx_t *ctx = (rootstream_ctx_t*)user_data;

    if (!ctx) return;

    /* Create dialog */
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Connect to Peer",
        NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Connect", GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, -1);

    /* Content area */
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 20);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    /* Instructions */
    GtkWidget *label = gtk_label_new(
        "Paste the RootStream code from the peer you want to connect to:");
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    /* Entry field */
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "kXx7Y...Qp9w==@gaming-pc");
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

    /* Example */
    GtkWidget *example = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(example),
        "<small><i>Format: base64_pubkey@hostname</i></small>");
    gtk_box_pack_start(GTK_BOX(vbox), example, FALSE, FALSE, 0);

    gtk_widget_show_all(content);

    /* Run dialog */
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response == GTK_RESPONSE_ACCEPT) {
        const char *code = gtk_entry_get_text(GTK_ENTRY(entry));
        
        if (code && strlen(code) > 0) {
            printf("INFO: Connecting to peer: %s\n", code);
            
            if (rootstream_connect_to_peer(ctx, code) == 0) {
                /* Show success notification */
                GtkWidget *msg = gtk_message_dialog_new(
                    NULL,
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_INFO,
                    GTK_BUTTONS_OK,
                    "Connection initiated to peer");
                gtk_dialog_run(GTK_DIALOG(msg));
                gtk_widget_destroy(msg);
            } else {
                /* Show error */
                GtkWidget *msg = gtk_message_dialog_new(
                    NULL,
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_OK,
                    "Failed to connect to peer.\n\n"
                    "Please check the RootStream code and try again.");
                gtk_dialog_run(GTK_DIALOG(msg));
                gtk_widget_destroy(msg);
            }
        }
    }

    gtk_widget_destroy(dialog);
}

/*
 * Show connected peers window
 */
static void on_view_peers(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    rootstream_ctx_t *ctx = (rootstream_ctx_t*)user_data;

    if (!ctx) return;

    /* Create window */
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Connected Peers");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    /* Scrolled window for list */
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(window), scroll);

    /* List box */
    GtkWidget *listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scroll), listbox);

    if (ctx->num_peers == 0) {
        /* No peers */
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new("No peers connected");
        gtk_container_add(GTK_CONTAINER(row), label);
        gtk_container_add(GTK_CONTAINER(listbox), row);
    } else {
        /* Add each peer */
        for (int i = 0; i < ctx->num_peers; i++) {
            peer_t *peer = &ctx->peers[i];

            GtkWidget *row = gtk_list_box_row_new();
            GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
            gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
            gtk_container_add(GTK_CONTAINER(row), hbox);

            /* Status icon */
            const char *status_icon = NULL;
            const char *status_text = NULL;
            switch (peer->state) {
                case PEER_CONNECTED:
                    status_icon = "✓";
                    status_text = "Connected";
                    break;
                case PEER_CONNECTING:
                    status_icon = "⋯";
                    status_text = "Connecting...";
                    break;
                case PEER_DISCOVERED:
                    status_icon = "○";
                    status_text = "Discovered";
                    break;
                case PEER_FAILED:
                    status_icon = "✗";
                    status_text = "Failed";
                    break;
                default:
                    status_icon = "✗";
                    status_text = "Disconnected";
                    break;
            }

            GtkWidget *icon = gtk_label_new(status_icon);
            gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);

            /* Peer info */
            GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
            gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

            char hostname_text[256];
            snprintf(hostname_text, sizeof(hostname_text), 
                     "<b>%s</b>", peer->hostname);
            GtkWidget *hostname = gtk_label_new(NULL);
            gtk_label_set_markup(GTK_LABEL(hostname), hostname_text);
            gtk_label_set_xalign(GTK_LABEL(hostname), 0);
            gtk_box_pack_start(GTK_BOX(vbox), hostname, FALSE, FALSE, 0);

            GtkWidget *status = gtk_label_new(status_text);
            gtk_label_set_xalign(GTK_LABEL(status), 0);
            gtk_box_pack_start(GTK_BOX(vbox), status, FALSE, FALSE, 0);

            gtk_container_add(GTK_CONTAINER(listbox), row);
        }
    }

    gtk_widget_show_all(window);
}

/*
 * Show settings dialog
 */
static void on_settings(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    rootstream_ctx_t *ctx = (rootstream_ctx_t*)user_data;

    /* Create settings dialog */
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "RootStream Settings",
        NULL,
        GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT,
        NULL
    );

    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 400);

    /* Create notebook for tabbed interface */
    GtkWidget *notebook = gtk_notebook_new();
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content), notebook, TRUE, TRUE, 0);

    /* --- Video Tab --- */
    GtkWidget *video_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(video_box), 15);

    /* Video bitrate */
    GtkWidget *bitrate_label = gtk_label_new("Bitrate (Mbps):");
    GtkWidget *bitrate_spin = gtk_spin_button_new_with_range(1, 50, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(bitrate_spin),
                              ctx->settings.video_bitrate / 1000000.0);
    gtk_box_pack_start(GTK_BOX(video_box), bitrate_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(video_box), bitrate_spin, FALSE, FALSE, 0);

    /* Video framerate */
    GtkWidget *fps_label = gtk_label_new("Framerate (FPS):");
    GtkWidget *fps_spin = gtk_spin_button_new_with_range(30, 144, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(fps_spin),
                              ctx->settings.video_framerate);
    gtk_box_pack_start(GTK_BOX(video_box), fps_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(video_box), fps_spin, FALSE, FALSE, 0);

    /* Video codec (label only for now, h264/h265 selection would go here) */
    char codec_text[64];
    snprintf(codec_text, sizeof(codec_text), "Codec: %s", ctx->settings.video_codec);
    GtkWidget *codec_label = gtk_label_new(codec_text);
    gtk_box_pack_start(GTK_BOX(video_box), codec_label, FALSE, FALSE, 0);

    /* Display selection */
    GtkWidget *display_label = gtk_label_new("Display:");
    GtkWidget *display_combo = gtk_combo_box_text_new();
    display_info_t displays[MAX_DISPLAYS];
    int num_displays = rootstream_detect_displays(displays, MAX_DISPLAYS);
    int active_index = 0;
    if (num_displays > 0) {
        for (int i = 0; i < num_displays; i++) {
            char item[128];
            snprintf(item, sizeof(item), "%d: %s (%dx%d @ %d Hz)",
                     i, displays[i].name, displays[i].width,
                     displays[i].height, displays[i].refresh_rate);
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(display_combo), item);
            if (i == ctx->settings.display_index) {
                active_index = i;
            }
        }
        for (int i = 0; i < num_displays; i++) {
            if (displays[i].fd >= 0) {
                close(displays[i].fd);
            }
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(display_combo), active_index);
    } else {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(display_combo),
                                       "No displays detected");
        gtk_combo_box_set_active(GTK_COMBO_BOX(display_combo), 0);
        gtk_widget_set_sensitive(display_combo, FALSE);
    }
    gtk_box_pack_start(GTK_BOX(video_box), display_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(video_box), display_combo, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), video_box,
                            gtk_label_new("Video"));

    /* --- Audio Tab --- */
    GtkWidget *audio_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(audio_box), 15);

    /* Audio enabled */
    GtkWidget *audio_enabled = gtk_check_button_new_with_label("Enable Audio");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(audio_enabled),
                                 ctx->settings.audio_enabled);
    gtk_box_pack_start(GTK_BOX(audio_box), audio_enabled, FALSE, FALSE, 0);

    /* Audio bitrate */
    GtkWidget *audio_bitrate_label = gtk_label_new("Audio Bitrate (kbps):");
    GtkWidget *audio_bitrate_spin = gtk_spin_button_new_with_range(32, 320, 8);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(audio_bitrate_spin),
                              ctx->settings.audio_bitrate / 1000.0);
    gtk_box_pack_start(GTK_BOX(audio_box), audio_bitrate_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(audio_box), audio_bitrate_spin, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), audio_box,
                            gtk_label_new("Audio"));

    /* --- Network Tab --- */
    GtkWidget *network_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(network_box), 15);

    /* Network port */
    GtkWidget *port_label = gtk_label_new("UDP Port:");
    GtkWidget *port_spin = gtk_spin_button_new_with_range(1024, 65535, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(port_spin),
                              ctx->settings.network_port);
    gtk_box_pack_start(GTK_BOX(network_box), port_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(network_box), port_spin, FALSE, FALSE, 0);

    /* Discovery enabled */
    GtkWidget *discovery_enabled = gtk_check_button_new_with_label("Enable mDNS Discovery");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(discovery_enabled),
                                 ctx->settings.discovery_enabled);
    gtk_box_pack_start(GTK_BOX(network_box), discovery_enabled, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), network_box,
                            gtk_label_new("Network"));

    /* Show all widgets */
    gtk_widget_show_all(dialog);

    /* Run dialog */
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response == GTK_RESPONSE_ACCEPT) {
        /* Save settings */
        ctx->settings.video_bitrate = (uint32_t)(
            gtk_spin_button_get_value(GTK_SPIN_BUTTON(bitrate_spin)) * 1000000);
        ctx->settings.video_framerate = (uint32_t)
            gtk_spin_button_get_value(GTK_SPIN_BUTTON(fps_spin));
        if (num_displays > 0) {
            ctx->settings.display_index =
                gtk_combo_box_get_active(GTK_COMBO_BOX(display_combo));
        }
        ctx->settings.audio_enabled =
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(audio_enabled));
        ctx->settings.audio_bitrate = (uint32_t)(
            gtk_spin_button_get_value(GTK_SPIN_BUTTON(audio_bitrate_spin)) * 1000);
        ctx->settings.network_port = (uint16_t)
            gtk_spin_button_get_value(GTK_SPIN_BUTTON(port_spin));
        ctx->settings.discovery_enabled =
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(discovery_enabled));

        /* Save to config file */
        config_save(ctx);

        printf("✓ Settings saved\n");
    }

    gtk_widget_destroy(dialog);
}

/*
 * Show about dialog
 */
static void on_about(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    (void)user_data;

    GtkWidget *dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "RootStream");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), ROOTSTREAM_VERSION);
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog),
        "Secure peer-to-peer game streaming\n"
        "Direct kernel access, no accounts, no BS");
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), 
        "https://github.com/yourusername/rootstream");
    gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dialog), GTK_LICENSE_MIT_X11);

    const char *authors[] = { "RootStream Contributors", NULL };
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/*
 * Quit application
 */
static void on_quit(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    rootstream_ctx_t *ctx = (rootstream_ctx_t*)user_data;
    
    if (ctx) {
        ctx->running = false;
    }
    
    gtk_main_quit();
}

/*
 * Create tray menu
 */
static void create_menu(rootstream_ctx_t *ctx, GtkStatusIcon *tray_icon) {
    (void)tray_icon;  /* Passed for potential future use */
    GtkWidget *menu = gtk_menu_new();

    /* My QR Code */
    GtkWidget *qr_item = gtk_menu_item_new_with_label("Show My QR Code");
    g_signal_connect(qr_item, "activate", G_CALLBACK(on_show_qr_code), ctx);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), qr_item);

    /* Connect to Peer */
    GtkWidget *connect_item = gtk_menu_item_new_with_label("Connect to Peer...");
    g_signal_connect(connect_item, "activate", G_CALLBACK(on_connect_to_peer), ctx);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), connect_item);

    /* View Peers */
    GtkWidget *peers_item = gtk_menu_item_new_with_label("View Peers");
    g_signal_connect(peers_item, "activate", G_CALLBACK(on_view_peers), ctx);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), peers_item);

    /* Separator */
    GtkWidget *sep1 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep1);

    /* Settings */
    GtkWidget *settings_item = gtk_menu_item_new_with_label("Settings...");
    g_signal_connect(settings_item, "activate", G_CALLBACK(on_settings), ctx);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), settings_item);

    /* About */
    GtkWidget *about_item = gtk_menu_item_new_with_label("About");
    g_signal_connect(about_item, "activate", G_CALLBACK(on_about), ctx);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), about_item);

    /* Separator */
    GtkWidget *sep2 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep2);

    /* Quit */
    GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit");
    g_signal_connect(quit_item, "activate", G_CALLBACK(on_quit), ctx);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit_item);

    gtk_widget_show_all(menu);

    /* Store menu in context */
    ctx->tray.menu = menu;
}

/*
 * Tray icon clicked/activated
 */
static void on_tray_activate(GtkStatusIcon *icon, gpointer user_data) {
    (void)icon;
    rootstream_ctx_t *ctx = (rootstream_ctx_t*)user_data;
    
    /* Left-click: show QR code */
    on_show_qr_code(NULL, ctx);
}

/*
 * Tray icon right-clicked (popup menu)
 */
static void on_tray_popup(GtkStatusIcon *icon, guint button, 
                         guint activate_time, gpointer user_data) {
    (void)icon;
    rootstream_ctx_t *ctx = (rootstream_ctx_t*)user_data;
    
    if (ctx->tray.menu) {
        gtk_menu_popup(GTK_MENU(ctx->tray.menu), NULL, NULL, NULL, NULL,
                      button, activate_time);
    }
}

/*
 * Initialize tray icon
 */
int tray_init(rootstream_ctx_t *ctx, int argc, char **argv) {
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context\n");
        return -1;
    }

    g_ctx = ctx;  /* Store for callbacks */

    /* Initialize GTK */
    gtk_init(&argc, &argv);

    /* Create status icon */
    GtkStatusIcon *tray_icon = gtk_status_icon_new_from_icon_name(ICON_IDLE);
    if (!tray_icon) {
        fprintf(stderr, "ERROR: Cannot create system tray icon\n");
        fprintf(stderr, "FIX: Ensure system tray is available\n");
        return -1;
    }

    gtk_status_icon_set_tooltip_text(tray_icon, "RootStream - Idle");
    gtk_status_icon_set_visible(tray_icon, TRUE);

    /* Connect signals */
    g_signal_connect(tray_icon, "activate", G_CALLBACK(on_tray_activate), ctx);
    g_signal_connect(tray_icon, "popup-menu", G_CALLBACK(on_tray_popup), ctx);

    /* Create menu */
    create_menu(ctx, tray_icon);

    ctx->tray.tray_icon = tray_icon;
    ctx->tray.status = STATUS_IDLE;

    printf("✓ System tray initialized\n");

    return 0;
}

/*
 * Update tray status (icon and tooltip)
 */
void tray_update_status(rootstream_ctx_t *ctx, tray_status_t status) {
    if (!ctx || !ctx->tray.tray_icon) return;

    GtkStatusIcon *icon = (GtkStatusIcon*)ctx->tray.tray_icon;
    ctx->tray.status = status;

    const char *icon_name = NULL;
    const char *tooltip = NULL;

    switch (status) {
        case STATUS_IDLE:
            icon_name = ICON_IDLE;
            tooltip = "RootStream - Idle";
            break;
        case STATUS_HOSTING:
            icon_name = ICON_HOSTING;
            tooltip = "RootStream - Hosting Stream";
            break;
        case STATUS_CONNECTED:
            icon_name = ICON_CONNECTED;
            tooltip = "RootStream - Connected";
            break;
        case STATUS_ERROR:
            icon_name = ICON_ERROR;
            tooltip = "RootStream - Error";
            break;
    }

    gtk_status_icon_set_from_icon_name(icon, icon_name);
    gtk_status_icon_set_tooltip_text(icon, tooltip);
}

/*
 * Run GTK main loop
 */
void tray_run(rootstream_ctx_t *ctx) {
    if (!ctx) return;
    
    /* This will block until gtk_main_quit() is called */
    gtk_main();
}

/*
 * Cleanup tray resources
 */
void tray_cleanup(rootstream_ctx_t *ctx) {
    if (!ctx) return;

    if (ctx->tray.tray_icon) {
        g_object_unref(ctx->tray.tray_icon);
        ctx->tray.tray_icon = NULL;
    }

    if (ctx->tray.menu) {
        gtk_widget_destroy(GTK_WIDGET(ctx->tray.menu));
        ctx->tray.menu = NULL;
    }
}
