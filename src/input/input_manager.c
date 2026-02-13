/*
 * input_manager.c - Multi-client input injection manager
 * 
 * Coordinates input from multiple clients with deduplication,
 * latency measurement, and backend abstraction.
 */

#include "../../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#ifdef __linux__
#include <linux/uinput.h>
#endif

/*
 * Emit an input event to a device
 */
static int emit_event(int fd, uint16_t type, uint16_t code, int32_t value) {
#ifdef __linux__
    struct input_event ev = {0};
    
    ev.type = type;
    ev.code = code;
    ev.value = value;
    
    /* Set timestamp */
    gettimeofday(&ev.time, NULL);
    
    if (write(fd, &ev, sizeof(ev)) < 0)
        return -1;
    
    /* Sync event */
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    
    if (write(fd, &ev, sizeof(ev)) < 0)
        return -1;
    
    return 0;
#else
    (void)fd;
    (void)type;
    (void)code;
    (void)value;
    return -1;
#endif
}

/*
 * Create a virtual keyboard device
 */
static int create_keyboard(void) {
#ifdef __linux__
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        return -1;
    }

    /* Enable key events */
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);

    /* Enable all keyboard keys */
    for (int i = 0; i < KEY_MAX; i++) {
        ioctl(fd, UI_SET_KEYBIT, i);
    }

    /* Setup device */
    struct uinput_setup setup = {0};
    snprintf(setup.name, UINPUT_MAX_NAME_SIZE, "RootStream Input Manager Keyboard");
    setup.id.bustype = BUS_USB;
    setup.id.vendor = 0x1234;
    setup.id.product = 0x5678;
    setup.id.version = 1;

    if (ioctl(fd, UI_DEV_SETUP, &setup) < 0) {
        close(fd);
        return -1;
    }

    /* Create the device */
    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        close(fd);
        return -1;
    }

    return fd;
#else
    return -1;
#endif
}

/*
 * Create a virtual mouse device
 */
static int create_mouse(void) {
#ifdef __linux__
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        return -1;
    }

    /* Enable events */
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);

    /* Enable mouse buttons */
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE);
    ioctl(fd, UI_SET_KEYBIT, BTN_SIDE);
    ioctl(fd, UI_SET_KEYBIT, BTN_EXTRA);

    /* Enable relative axes */
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);
    ioctl(fd, UI_SET_RELBIT, REL_WHEEL);
    ioctl(fd, UI_SET_RELBIT, REL_HWHEEL);

    /* Setup device */
    struct uinput_setup setup = {0};
    snprintf(setup.name, UINPUT_MAX_NAME_SIZE, "RootStream Input Manager Mouse");
    setup.id.bustype = BUS_USB;
    setup.id.vendor = 0x1234;
    setup.id.product = 0x5679;
    setup.id.version = 1;

    if (ioctl(fd, UI_DEV_SETUP, &setup) < 0) {
        close(fd);
        return -1;
    }

    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        close(fd);
        return -1;
    }

    return fd;
#else
    return -1;
#endif
}

/*
 * Create a virtual gamepad device
 */
static int create_gamepad(void) {
#ifdef __linux__
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        return -1;
    }

    /* Enable events */
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);

    /* Enable gamepad buttons */
    ioctl(fd, UI_SET_KEYBIT, BTN_SOUTH);     /* A */
    ioctl(fd, UI_SET_KEYBIT, BTN_EAST);      /* B */
    ioctl(fd, UI_SET_KEYBIT, BTN_WEST);      /* X */
    ioctl(fd, UI_SET_KEYBIT, BTN_NORTH);     /* Y */
    ioctl(fd, UI_SET_KEYBIT, BTN_TL);        /* L1 */
    ioctl(fd, UI_SET_KEYBIT, BTN_TR);        /* R1 */
    ioctl(fd, UI_SET_KEYBIT, BTN_SELECT);    /* Back */
    ioctl(fd, UI_SET_KEYBIT, BTN_START);     /* Start */
    ioctl(fd, UI_SET_KEYBIT, BTN_THUMBL);    /* L3 */
    ioctl(fd, UI_SET_KEYBIT, BTN_THUMBR);    /* R3 */

    /* Enable analog sticks and triggers */
    ioctl(fd, UI_SET_ABSBIT, ABS_X);         /* Left stick X */
    ioctl(fd, UI_SET_ABSBIT, ABS_Y);         /* Left stick Y */
    ioctl(fd, UI_SET_ABSBIT, ABS_RX);        /* Right stick X */
    ioctl(fd, UI_SET_ABSBIT, ABS_RY);        /* Right stick Y */
    ioctl(fd, UI_SET_ABSBIT, ABS_Z);         /* Left trigger */
    ioctl(fd, UI_SET_ABSBIT, ABS_RZ);        /* Right trigger */

    /* Setup device */
    struct uinput_setup setup = {0};
    snprintf(setup.name, UINPUT_MAX_NAME_SIZE, "RootStream Input Manager Gamepad");
    setup.id.bustype = BUS_USB;
    setup.id.vendor = 0x045e;  /* Microsoft */
    setup.id.product = 0x028e; /* Xbox 360 Controller */
    setup.id.version = 1;

    if (ioctl(fd, UI_DEV_SETUP, &setup) < 0) {
        close(fd);
        return -1;
    }

    /* Setup absolute axis ranges */
    struct uinput_abs_setup abs_setup;
    
    /* Left stick X */
    abs_setup.code = ABS_X;
    abs_setup.absinfo.minimum = -32768;
    abs_setup.absinfo.maximum = 32767;
    abs_setup.absinfo.value = 0;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    /* Left stick Y */
    abs_setup.code = ABS_Y;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    /* Right stick X */
    abs_setup.code = ABS_RX;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    /* Right stick Y */
    abs_setup.code = ABS_RY;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    /* Triggers (0-255) */
    abs_setup.code = ABS_Z;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = 255;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    abs_setup.code = ABS_RZ;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);

    /* Create the device */
    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        close(fd);
        return -1;
    }

    return fd;
#else
    return -1;
#endif
}

/*
 * Check if an event is a duplicate
 */
static bool is_duplicate_event(input_manager_ctx_t *mgr, uint32_t client_id,
                               uint16_t sequence_number) {
    for (int i = 0; i < INPUT_MAX_CLIENTS; i++) {
        if (mgr->clients[i].active && mgr->clients[i].client_id == client_id) {
            /* Check if we've already seen this sequence number */
            if (mgr->clients[i].last_sequence_number == sequence_number) {
                return true;
            }
            return false;
        }
    }
    return false;
}

/*
 * Update client tracking after processing event
 */
static void update_client_tracking(input_manager_ctx_t *mgr, uint32_t client_id,
                                   uint16_t sequence_number, uint64_t timestamp_us) {
    for (int i = 0; i < INPUT_MAX_CLIENTS; i++) {
        if (mgr->clients[i].active && mgr->clients[i].client_id == client_id) {
            mgr->clients[i].last_sequence_number = sequence_number;
            mgr->clients[i].last_event_timestamp_us = timestamp_us;
            mgr->clients[i].event_count++;
            return;
        }
    }
}

/*
 * Process an input event
 */
static int process_input_event(input_manager_ctx_t *mgr, const input_event_pkt_t *event) {
    if (!mgr || !event) {
        return -1;
    }

    int result = 0;
    
    switch (event->type) {
#ifdef __linux__
        case EV_KEY:
            /* Keyboard or mouse/gamepad button */
            if (event->code < BTN_MOUSE) {
                /* Keyboard */
                if (mgr->device_fd_kbd >= 0) {
                    result = emit_event(mgr->device_fd_kbd, EV_KEY,
                                      event->code, event->value);
                }
            } else if (event->code < BTN_JOYSTICK) {
                /* Mouse button */
                if (mgr->device_fd_mouse >= 0) {
                    result = emit_event(mgr->device_fd_mouse, EV_KEY,
                                      event->code, event->value);
                }
            } else {
                /* Gamepad button */
                if (mgr->device_fd_gamepad >= 0) {
                    result = emit_event(mgr->device_fd_gamepad, EV_KEY,
                                      event->code, event->value);
                }
            }
            break;

        case EV_REL:
            /* Mouse movement */
            if (mgr->device_fd_mouse >= 0) {
                result = emit_event(mgr->device_fd_mouse, EV_REL,
                                  event->code, event->value);
            }
            break;

        case EV_ABS:
            /* Gamepad analog axes */
            if (mgr->device_fd_gamepad >= 0) {
                result = emit_event(mgr->device_fd_gamepad, EV_ABS,
                                  event->code, event->value);
            }
            break;
#endif

        default:
            /* Unsupported event type */
            result = 0;
            break;
    }

    return result;
}

/*
 * Initialize input manager
 */
int input_manager_init(rootstream_ctx_t *ctx, input_backend_type_t backend) {
    if (!ctx) {
        fprintf(stderr, "Invalid context\n");
        return -1;
    }

    /* Allocate manager context */
    ctx->input_manager = calloc(1, sizeof(input_manager_ctx_t));
    if (!ctx->input_manager) {
        fprintf(stderr, "Failed to allocate input manager\n");
        return -1;
    }

    input_manager_ctx_t *mgr = ctx->input_manager;
    mgr->backend_type = backend;
    mgr->device_fd_kbd = -1;
    mgr->device_fd_mouse = -1;
    mgr->device_fd_gamepad = -1;

    /* Initialize based on backend type */
    switch (backend) {
        case INPUT_BACKEND_UINPUT:
            /* Create virtual devices */
            mgr->device_fd_kbd = create_keyboard();
            if (mgr->device_fd_kbd < 0) {
                fprintf(stderr, "Warning: Cannot create virtual keyboard: %s\n",
                       strerror(errno));
            }

            mgr->device_fd_mouse = create_mouse();
            if (mgr->device_fd_mouse < 0) {
                fprintf(stderr, "Warning: Cannot create virtual mouse: %s\n",
                       strerror(errno));
            }

            mgr->device_fd_gamepad = create_gamepad();
            if (mgr->device_fd_gamepad < 0) {
                fprintf(stderr, "Warning: Cannot create virtual gamepad: %s\n",
                       strerror(errno));
            }

            if (mgr->device_fd_kbd >= 0 || mgr->device_fd_mouse >= 0 ||
                mgr->device_fd_gamepad >= 0) {
                printf("✓ Input Manager: uinput devices created\n");
                mgr->initialized = true;
            } else {
                fprintf(stderr, "Failed to create any virtual input devices\n");
                free(ctx->input_manager);
                ctx->input_manager = NULL;
                return -1;
            }
            break;

        case INPUT_BACKEND_XDOTOOL:
            /* Use existing xdotool backend */
            if (input_xdotool_available()) {
                if (input_init_xdotool(ctx) == 0) {
                    printf("✓ Input Manager: xdotool backend initialized\n");
                    mgr->initialized = true;
                } else {
                    fprintf(stderr, "Failed to initialize xdotool backend\n");
                    free(ctx->input_manager);
                    ctx->input_manager = NULL;
                    return -1;
                }
            } else {
                fprintf(stderr, "xdotool not available\n");
                free(ctx->input_manager);
                ctx->input_manager = NULL;
                return -1;
            }
            break;

        case INPUT_BACKEND_LOGGING:
            /* Use logging-only backend */
            if (input_init_logging(ctx) == 0) {
                printf("✓ Input Manager: logging backend initialized\n");
                mgr->initialized = true;
            } else {
                fprintf(stderr, "Failed to initialize logging backend\n");
                free(ctx->input_manager);
                ctx->input_manager = NULL;
                return -1;
            }
            break;

        default:
            fprintf(stderr, "Unknown input backend type\n");
            free(ctx->input_manager);
            ctx->input_manager = NULL;
            return -1;
    }

    /* Update active backend name */
    switch (backend) {
        case INPUT_BACKEND_UINPUT:
            ctx->active_backend.input_name = "uinput";
            break;
        case INPUT_BACKEND_XDOTOOL:
            ctx->active_backend.input_name = "xdotool";
            break;
        case INPUT_BACKEND_LOGGING:
            ctx->active_backend.input_name = "logging";
            break;
    }

    return 0;
}

/*
 * Submit an input packet for processing
 */
int input_manager_submit_packet(rootstream_ctx_t *ctx, const input_event_pkt_t *event,
                                uint32_t client_id, uint16_t sequence_number,
                                uint64_t timestamp_us) {
    if (!ctx || !ctx->input_manager || !event) {
        return -1;
    }

    input_manager_ctx_t *mgr = ctx->input_manager;

    if (!mgr->initialized) {
        return -1;
    }

    /* Check for duplicate */
    if (is_duplicate_event(mgr, client_id, sequence_number)) {
        mgr->duplicate_inputs_detected++;
        return 0;  /* Not an error, just skip duplicate */
    }

    /* Record receive time for latency measurement */
    uint64_t receive_time = get_timestamp_us();

    /* Process the event based on backend */
    int result = 0;
    
    switch (mgr->backend_type) {
        case INPUT_BACKEND_UINPUT:
            result = process_input_event(mgr, event);
            break;

        case INPUT_BACKEND_XDOTOOL:
            /* Use xdotool backend functions */
            if (event->type == EV_KEY && event->code < BTN_MOUSE) {
                result = input_inject_key_xdotool(event->code, event->value != 0);
            } else if (event->type == EV_REL || event->type == EV_KEY) {
                /* For mouse events, xdotool needs different handling */
                result = 0;  /* Simplified for now */
            }
            break;

        case INPUT_BACKEND_LOGGING:
            /* Use logging backend functions */
            if (event->type == EV_KEY && event->code < BTN_MOUSE) {
                result = input_inject_key_logging(event->code, event->value != 0);
            } else {
                result = 0;  /* Log only */
            }
            break;
    }

    if (result == 0) {
        /* Update tracking */
        update_client_tracking(mgr, client_id, sequence_number, timestamp_us);
        mgr->total_inputs_processed++;

        /* Calculate latency if client timestamp is valid */
        if (timestamp_us > 0) {
            uint64_t latency = receive_time - timestamp_us;
            mgr->total_latency_us += latency;
            mgr->latency_samples++;
        }
    }

    return result;
}

/*
 * Register a client
 */
int input_manager_register_client(rootstream_ctx_t *ctx, uint32_t client_id,
                                  const char *client_name) {
    if (!ctx || !ctx->input_manager) {
        return -1;
    }

    input_manager_ctx_t *mgr = ctx->input_manager;

    /* Find available slot */
    for (int i = 0; i < INPUT_MAX_CLIENTS; i++) {
        if (!mgr->clients[i].active) {
            mgr->clients[i].client_id = client_id;
            if (client_name) {
                strncpy(mgr->clients[i].client_name, client_name, 63);
                mgr->clients[i].client_name[63] = '\0';
            } else {
                snprintf(mgr->clients[i].client_name, 64, "Client-%u", client_id);
            }
            mgr->clients[i].active = true;
            mgr->clients[i].event_count = 0;
            mgr->clients[i].last_sequence_number = 0xFFFF;  /* Sentinel - no sequence seen yet */
            mgr->active_client_count++;
            
            printf("Input Manager: Registered client %u (%s)\n",
                   client_id, mgr->clients[i].client_name);
            return 0;
        }
    }

    fprintf(stderr, "Input Manager: Max clients reached\n");
    return -1;
}

/*
 * Unregister a client
 */
int input_manager_unregister_client(rootstream_ctx_t *ctx, uint32_t client_id) {
    if (!ctx || !ctx->input_manager) {
        return -1;
    }

    input_manager_ctx_t *mgr = ctx->input_manager;

    for (int i = 0; i < INPUT_MAX_CLIENTS; i++) {
        if (mgr->clients[i].active && mgr->clients[i].client_id == client_id) {
            printf("Input Manager: Unregistered client %u (%s)\n",
                   client_id, mgr->clients[i].client_name);
            
            memset(&mgr->clients[i], 0, sizeof(input_client_info_t));
            mgr->active_client_count--;
            return 0;
        }
    }

    return -1;
}

/*
 * Get average input latency in milliseconds
 */
uint32_t input_manager_get_latency_ms(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->input_manager) {
        return 0;
    }

    input_manager_ctx_t *mgr = ctx->input_manager;

    if (mgr->latency_samples == 0) {
        return 0;
    }

    uint64_t avg_latency_us = mgr->total_latency_us / mgr->latency_samples;
    return (uint32_t)(avg_latency_us / 1000);
}

/*
 * Get total inputs processed
 */
uint32_t input_manager_get_total_inputs(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->input_manager) {
        return 0;
    }

    return (uint32_t)ctx->input_manager->total_inputs_processed;
}

/*
 * Get duplicate inputs detected
 */
uint32_t input_manager_get_duplicates(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->input_manager) {
        return 0;
    }

    return (uint32_t)ctx->input_manager->duplicate_inputs_detected;
}

/*
 * Cleanup input manager
 */
void input_manager_cleanup(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->input_manager) {
        return;
    }

    input_manager_ctx_t *mgr = ctx->input_manager;

#ifdef __linux__
    if (mgr->device_fd_kbd >= 0) {
        ioctl(mgr->device_fd_kbd, UI_DEV_DESTROY);
        close(mgr->device_fd_kbd);
    }

    if (mgr->device_fd_mouse >= 0) {
        ioctl(mgr->device_fd_mouse, UI_DEV_DESTROY);
        close(mgr->device_fd_mouse);
    }

    if (mgr->device_fd_gamepad >= 0) {
        ioctl(mgr->device_fd_gamepad, UI_DEV_DESTROY);
        close(mgr->device_fd_gamepad);
    }
#endif

    /* Cleanup backend if needed */
    switch (mgr->backend_type) {
        case INPUT_BACKEND_XDOTOOL:
            input_cleanup_xdotool(ctx);
            break;
        case INPUT_BACKEND_LOGGING:
            input_cleanup_logging(ctx);
            break;
        default:
            break;
    }

    free(ctx->input_manager);
    ctx->input_manager = NULL;

    printf("✓ Input Manager cleaned up\n");
}
