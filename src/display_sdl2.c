/*
 * display_sdl2.c - SDL2 display backend for video playback
 *
 * Simple SDL2-based video renderer for the client.
 * Handles window creation, frame presentation, and basic event handling.
 *
 * Architecture:
 * - Create SDL window and renderer
 * - Create texture for YUV (NV12) frames
 * - Update texture with decoded frame data
 * - Present to screen with vsync
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* SDL2 headers */
#include <SDL2/SDL.h>

/* Linux input event types */
#include <linux/input.h>

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int width;
    int height;
    bool initialized;
} sdl2_display_ctx_t;

/* Forward SDL2 input event to host */
static void forward_input_event(rootstream_ctx_t *ctx, uint8_t type,
                                uint16_t code, int32_t value) {
    if (!ctx || ctx->num_peers == 0) {
        return;
    }

    /* Create input event packet */
    input_event_pkt_t event_pkt = {
        .type = type,
        .code = code,
        .value = value
    };

    /* Send to first connected peer (typically there's only one for client) */
    for (int i = 0; i < ctx->num_peers; i++) {
        peer_t *peer = &ctx->peers[i];
        if (peer->state == PEER_CONNECTED) {
            rootstream_net_send_encrypted(ctx, peer, PKT_INPUT,
                                         &event_pkt, sizeof(event_pkt));
            break;
        }
    }
}

/* Convert SDL2 keycode to Linux key code */
static uint16_t sdl_to_linux_keycode(SDL_Keycode sdl_key) {
    /* Common key mappings (simplified - full mapping would be much longer) */
    switch (sdl_key) {
        case SDLK_ESCAPE: return KEY_ESC;
        case SDLK_RETURN: return KEY_ENTER;
        case SDLK_BACKSPACE: return KEY_BACKSPACE;
        case SDLK_TAB: return KEY_TAB;
        case SDLK_SPACE: return KEY_SPACE;
        case SDLK_LEFT: return KEY_LEFT;
        case SDLK_RIGHT: return KEY_RIGHT;
        case SDLK_UP: return KEY_UP;
        case SDLK_DOWN: return KEY_DOWN;
        case SDLK_LSHIFT: return KEY_LEFTSHIFT;
        case SDLK_RSHIFT: return KEY_RIGHTSHIFT;
        case SDLK_LCTRL: return KEY_LEFTCTRL;
        case SDLK_RCTRL: return KEY_RIGHTCTRL;
        case SDLK_LALT: return KEY_LEFTALT;
        case SDLK_RALT: return KEY_RIGHTALT;

        /* Letters (a-z) */
        default:
            if (sdl_key >= SDLK_a && sdl_key <= SDLK_z) {
                return KEY_A + (sdl_key - SDLK_a);
            }
            /* Numbers (0-9) */
            if (sdl_key >= SDLK_0 && sdl_key <= SDLK_9) {
                return KEY_0 + (sdl_key - SDLK_0);
            }
            /* F keys (F1-F12) */
            if (sdl_key >= SDLK_F1 && sdl_key <= SDLK_F12) {
                return KEY_F1 + (sdl_key - SDLK_F1);
            }
            return 0;  /* Unknown key */
    }
}

/*
 * Initialize SDL2 display
 *
 * @param ctx    RootStream context
 * @param title  Window title
 * @param width  Window width
 * @param height Window height
 * @return       0 on success, -1 on error
 */
int display_init(rootstream_ctx_t *ctx, const char *title,
                int width, int height) {
    if (!ctx || !title) {
        fprintf(stderr, "ERROR: Invalid arguments to display_init\n");
        return -1;
    }

    /* Initialize SDL video subsystem */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "ERROR: SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    printf("✓ SDL2 initialized\n");

    /* Allocate display context */
    sdl2_display_ctx_t *disp = calloc(1, sizeof(sdl2_display_ctx_t));
    if (!disp) {
        SDL_Quit();
        fprintf(stderr, "ERROR: Cannot allocate display context\n");
        return -1;
    }

    disp->width = width;
    disp->height = height;

    /* Create window */
    disp->window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!disp->window) {
        fprintf(stderr, "ERROR: SDL_CreateWindow failed: %s\n", SDL_GetError());
        free(disp);
        SDL_Quit();
        return -1;
    }

    /* Create renderer with vsync for smooth playback */
    disp->renderer = SDL_CreateRenderer(
        disp->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!disp->renderer) {
        fprintf(stderr, "ERROR: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(disp->window);
        free(disp);
        SDL_Quit();
        return -1;
    }

    /* Create texture for YUV (NV12) frames */
    disp->texture = SDL_CreateTexture(
        disp->renderer,
        SDL_PIXELFORMAT_NV12,  /* NV12 format from VA-API decoder */
        SDL_TEXTUREACCESS_STREAMING,
        width, height
    );

    if (!disp->texture) {
        fprintf(stderr, "ERROR: SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(disp->renderer);
        SDL_DestroyWindow(disp->window);
        free(disp);
        SDL_Quit();
        return -1;
    }

    disp->initialized = true;

    /* Store display context (reuse tray context pointer) */
    ctx->tray.gtk_app = disp;

    printf("✓ SDL2 display ready: %dx%d\n", width, height);

    return 0;
}

/*
 * Present a decoded frame to the display
 *
 * @param ctx   RootStream context
 * @param frame Decoded frame (NV12 format)
 * @return      0 on success, -1 on error
 */
int display_present_frame(rootstream_ctx_t *ctx, frame_buffer_t *frame) {
    if (!ctx || !frame || !frame->data) {
        return -1;
    }

    sdl2_display_ctx_t *disp = (sdl2_display_ctx_t*)ctx->tray.gtk_app;
    if (!disp || !disp->initialized) {
        fprintf(stderr, "ERROR: Display not initialized\n");
        return -1;
    }

    /* Update texture with frame data (NV12 format) */
    int ret = SDL_UpdateTexture(
        disp->texture,
        NULL,  /* Update entire texture */
        frame->data,
        frame->pitch
    );

    if (ret < 0) {
        fprintf(stderr, "ERROR: SDL_UpdateTexture failed: %s\n", SDL_GetError());
        return -1;
    }

    /* Clear renderer */
    SDL_RenderClear(disp->renderer);

    /* Copy texture to renderer */
    SDL_RenderCopy(disp->renderer, disp->texture, NULL, NULL);

    /* Present to screen (waits for vsync) */
    SDL_RenderPresent(disp->renderer);

    return 0;
}

/*
 * Poll SDL2 events (keyboard, mouse, window events)
 *
 * @param ctx RootStream context
 * @return    1 if should quit, 0 otherwise
 *
 * Note: This is a basic implementation. Phase 4 will add
 * proper input capture and forwarding to host.
 */
int display_poll_events(rootstream_ctx_t *ctx) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                /* User closed window */
                return 1;

            case SDL_KEYDOWN:
                /* ESC key to quit (don't forward) */
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    return 1;
                }
                /* Forward key press to host */
                {
                    uint16_t linux_key = sdl_to_linux_keycode(event.key.keysym.sym);
                    if (linux_key != 0) {
                        forward_input_event(ctx, EV_KEY, linux_key, 1);  /* 1 = pressed */
                    }
                }
                break;

            case SDL_KEYUP:
                /* Forward key release to host */
                {
                    uint16_t linux_key = sdl_to_linux_keycode(event.key.keysym.sym);
                    if (linux_key != 0) {
                        forward_input_event(ctx, EV_KEY, linux_key, 0);  /* 0 = released */
                    }
                }
                break;

            case SDL_MOUSEMOTION:
                /* Forward relative mouse movement */
                forward_input_event(ctx, EV_REL, REL_X, event.motion.xrel);
                forward_input_event(ctx, EV_REL, REL_Y, event.motion.yrel);
                break;

            case SDL_MOUSEBUTTONDOWN:
                /* Forward mouse button press */
                {
                    uint16_t btn = BTN_LEFT;
                    if (event.button.button == SDL_BUTTON_LEFT) btn = BTN_LEFT;
                    else if (event.button.button == SDL_BUTTON_RIGHT) btn = BTN_RIGHT;
                    else if (event.button.button == SDL_BUTTON_MIDDLE) btn = BTN_MIDDLE;
                    forward_input_event(ctx, EV_KEY, btn, 1);  /* 1 = pressed */
                }
                break;

            case SDL_MOUSEBUTTONUP:
                /* Forward mouse button release */
                {
                    uint16_t btn = BTN_LEFT;
                    if (event.button.button == SDL_BUTTON_LEFT) btn = BTN_LEFT;
                    else if (event.button.button == SDL_BUTTON_RIGHT) btn = BTN_RIGHT;
                    else if (event.button.button == SDL_BUTTON_MIDDLE) btn = BTN_MIDDLE;
                    forward_input_event(ctx, EV_KEY, btn, 0);  /* 0 = released */
                }
                break;

            case SDL_MOUSEWHEEL:
                /* Forward mouse wheel */
                if (event.wheel.y != 0) {
                    forward_input_event(ctx, EV_REL, REL_WHEEL, event.wheel.y);
                }
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    /* Window resized - texture will scale automatically */
                    printf("INFO: Window resized to %dx%d\n",
                           event.window.data1, event.window.data2);
                }
                break;

            default:
                break;
        }
    }

    return 0;
}

/*
 * Cleanup display resources
 */
void display_cleanup(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->tray.gtk_app) {
        return;
    }

    sdl2_display_ctx_t *disp = (sdl2_display_ctx_t*)ctx->tray.gtk_app;

    if (disp->texture) {
        SDL_DestroyTexture(disp->texture);
    }

    if (disp->renderer) {
        SDL_DestroyRenderer(disp->renderer);
    }

    if (disp->window) {
        SDL_DestroyWindow(disp->window);
    }

    free(disp);
    ctx->tray.gtk_app = NULL;

    SDL_Quit();

    printf("✓ Display cleanup complete\n");
}
