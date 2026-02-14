/**
 * @file client_input_x11.c
 * @brief X11-based input capture implementation
 */

#include "client_input.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

// X11 headers (only if available)
#if __has_include(<X11/Xlib.h>)
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#define HAVE_X11_HEADERS 1
#endif

/**
 * X11 input context structure
 */
struct client_input_ctx {
#ifdef HAVE_X11_HEADERS
    Display *display;
    Window window;
#else
    void *display;
    void *window;
#endif
    
    input_event_callback_t callback;
    void *user_data;
    
    bool capturing;
    bool mouse_captured;
    int last_mouse_x;
    int last_mouse_y;
    
    char last_error[256];
};

/**
 * Get current timestamp in microseconds
 */
static uint64_t get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

/**
 * Map X11 KeySym to Linux key code
 * This is a simplified mapping - full implementation would need a complete table
 */
static uint16_t xkeysym_to_linux_keycode(unsigned long keysym) {
#ifndef HAVE_X11_HEADERS
    (void)keysym;
    return 0;
#else
    // Simplified mapping for common keys
    // Full implementation would use a complete translation table
    
    // Letters (A-Z)
    if (keysym >= XK_a && keysym <= XK_z) {
        return 30 + (keysym - XK_a);  // KEY_A = 30
    }
    if (keysym >= XK_A && keysym <= XK_Z) {
        return 30 + (keysym - XK_A);
    }
    
    // Numbers (0-9)
    if (keysym >= XK_0 && keysym <= XK_9) {
        return 11 + (keysym - XK_0);  // KEY_0 = 11
    }
    
    // Function keys
    if (keysym >= XK_F1 && keysym <= XK_F12) {
        return 59 + (keysym - XK_F1);  // KEY_F1 = 59
    }
    
    // Special keys
    switch (keysym) {
        case XK_Escape:     return 1;    // KEY_ESC
        case XK_Return:     return 28;   // KEY_ENTER
        case XK_space:      return 57;   // KEY_SPACE
        case XK_BackSpace:  return 14;   // KEY_BACKSPACE
        case XK_Tab:        return 15;   // KEY_TAB
        case XK_Shift_L:    return 42;   // KEY_LEFTSHIFT
        case XK_Shift_R:    return 54;   // KEY_RIGHTSHIFT
        case XK_Control_L:  return 29;   // KEY_LEFTCTRL
        case XK_Control_R:  return 97;   // KEY_RIGHTCTRL
        case XK_Alt_L:      return 56;   // KEY_LEFTALT
        case XK_Alt_R:      return 100;  // KEY_RIGHTALT
        case XK_Left:       return 105;  // KEY_LEFT
        case XK_Right:      return 106;  // KEY_RIGHT
        case XK_Up:         return 103;  // KEY_UP
        case XK_Down:       return 108;  // KEY_DOWN
        default:            return 0;    // Unknown
    }
#endif
}

client_input_ctx_t* client_input_init(input_event_callback_t callback, void *user_data) {
    if (!callback) {
        return NULL;
    }
    
    client_input_ctx_t *ctx = calloc(1, sizeof(client_input_ctx_t));
    if (!ctx) {
        return NULL;
    }
    
    ctx->callback = callback;
    ctx->user_data = user_data;
    ctx->capturing = false;
    ctx->mouse_captured = false;
    
    return ctx;
}

int client_input_start_capture(client_input_ctx_t *ctx, void *native_window) {
    if (!ctx) {
        return -1;
    }
    
#ifndef HAVE_X11_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "X11 headers not available at compile time");
    return -1;
#else
    // Get X11 display and window
    if (!native_window) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "No window provided");
        return -1;
    }
    
    Window window = *(Window*)native_window;
    Display *display = XOpenDisplay(NULL);
    
    if (!display) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                "Failed to open X11 display");
        return -1;
    }
    
    // Select input events
    XSelectInput(display, window,
                KeyPressMask | KeyReleaseMask |
                ButtonPressMask | ButtonReleaseMask |
                PointerMotionMask |
                FocusChangeMask);
    
    ctx->display = display;
    ctx->window = window;
    ctx->capturing = true;
    
    printf("Input capture started on X11 window\n");
    
    return 0;
#endif
}

void client_input_stop_capture(client_input_ctx_t *ctx) {
    if (!ctx || !ctx->capturing) {
        return;
    }
    
#ifdef HAVE_X11_HEADERS
    if (ctx->mouse_captured) {
        client_input_set_mouse_capture(ctx, false);
    }
    
    if (ctx->display) {
        XCloseDisplay(ctx->display);
        ctx->display = NULL;
    }
#endif
    
    ctx->capturing = false;
    printf("Input capture stopped\n");
}

int client_input_process_events(client_input_ctx_t *ctx) {
    if (!ctx || !ctx->capturing) {
        return 0;
    }
    
#ifndef HAVE_X11_HEADERS
    return 0;
#else
    Display *display = ctx->display;
    int event_count = 0;
    
    // Process all pending X11 events
    while (XPending(display) > 0) {
        XEvent xevent;
        XNextEvent(display, &xevent);
        
        client_input_event_t event = {0};
        event.timestamp_us = get_timestamp_us();
        
        switch (xevent.type) {
            case KeyPress:
            case KeyRelease: {
                KeySym keysym = XLookupKeysym(&xevent.xkey, 0);
                uint16_t keycode = xkeysym_to_linux_keycode(keysym);
                
                if (keycode > 0) {
                    event.type = EV_KEY;
                    event.code = keycode;
                    event.value = (xevent.type == KeyPress) ? 1 : 0;
                    
                    ctx->callback(&event, ctx->user_data);
                    event_count++;
                }
                break;
            }
            
            case ButtonPress:
            case ButtonRelease: {
                event.type = EV_KEY;
                
                // Map X11 buttons to Linux button codes
                switch (xevent.xbutton.button) {
                    case Button1: event.code = BTN_LEFT; break;
                    case Button2: event.code = BTN_MIDDLE; break;
                    case Button3: event.code = BTN_RIGHT; break;
                    case Button4: // Scroll up
                        if (xevent.type == ButtonPress) {
                            event.type = EV_REL;
                            event.code = REL_WHEEL;
                            event.value = 1;
                        }
                        break;
                    case Button5: // Scroll down
                        if (xevent.type == ButtonPress) {
                            event.type = EV_REL;
                            event.code = REL_WHEEL;
                            event.value = -1;
                        }
                        break;
                    default:
                        continue;
                }
                
                if (event.code != 0) {
                    event.value = (xevent.type == ButtonPress) ? 1 : 0;
                    ctx->callback(&event, ctx->user_data);
                    event_count++;
                }
                break;
            }
            
            case MotionNotify: {
                int dx = xevent.xmotion.x - ctx->last_mouse_x;
                int dy = xevent.xmotion.y - ctx->last_mouse_y;
                
                ctx->last_mouse_x = xevent.xmotion.x;
                ctx->last_mouse_y = xevent.xmotion.y;
                
                // Send X motion
                if (dx != 0) {
                    event.type = EV_REL;
                    event.code = REL_X;
                    event.value = dx;
                    ctx->callback(&event, ctx->user_data);
                    event_count++;
                }
                
                // Send Y motion
                if (dy != 0) {
                    event.type = EV_REL;
                    event.code = REL_Y;
                    event.value = dy;
                    ctx->callback(&event, ctx->user_data);
                    event_count++;
                }
                break;
            }
            
            default:
                break;
        }
    }
    
    return event_count;
#endif
}

int client_input_set_mouse_capture(client_input_ctx_t *ctx, bool enable) {
    if (!ctx || !ctx->capturing) {
        return -1;
    }
    
#ifndef HAVE_X11_HEADERS
    snprintf(ctx->last_error, sizeof(ctx->last_error),
            "X11 headers not available at compile time");
    return -1;
#else
    Display *display = ctx->display;
    Window window = ctx->window;
    
    if (enable && !ctx->mouse_captured) {
        // Grab pointer
        int result = XGrabPointer(display, window, True,
                                 ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                                 GrabModeAsync, GrabModeAsync,
                                 window, None, CurrentTime);
        
        if (result == GrabSuccess) {
            // Hide cursor
            Cursor invisible_cursor;
            Pixmap bitmapNoData;
            XColor black;
            static char noData[] = { 0,0,0,0,0,0,0,0 };
            black.red = black.green = black.blue = 0;
            
            bitmapNoData = XCreateBitmapFromData(display, window, noData, 8, 8);
            invisible_cursor = XCreatePixmapCursor(display, bitmapNoData, bitmapNoData,
                                                  &black, &black, 0, 0);
            XDefineCursor(display, window, invisible_cursor);
            XFreeCursor(display, invisible_cursor);
            XFreePixmap(display, bitmapNoData);
            
            ctx->mouse_captured = true;
            printf("Mouse captured\n");
            return 0;
        } else {
            snprintf(ctx->last_error, sizeof(ctx->last_error),
                    "Failed to grab pointer: %d", result);
            return -1;
        }
    } else if (!enable && ctx->mouse_captured) {
        // Ungrab pointer
        XUngrabPointer(display, CurrentTime);
        XUndefineCursor(display, window);
        
        ctx->mouse_captured = false;
        printf("Mouse released\n");
        return 0;
    }
    
    return 0;
#endif
}

bool client_input_is_mouse_captured(client_input_ctx_t *ctx) {
    if (!ctx) {
        return false;
    }
    return ctx->mouse_captured;
}

void client_input_cleanup(client_input_ctx_t *ctx) {
    if (!ctx) {
        return;
    }
    
    client_input_stop_capture(ctx);
    free(ctx);
}

const char* client_input_get_error(client_input_ctx_t *ctx) {
    if (!ctx) {
        return NULL;
    }
    return ctx->last_error[0] ? ctx->last_error : NULL;
}
