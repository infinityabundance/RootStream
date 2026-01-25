/*
 * input.c - uinput-based input injection
 * 
 * Creates virtual keyboard and mouse devices to inject input
 * from the remote client. Works regardless of display server.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <linux/uinput.h>

/*
 * Create a virtual keyboard device
 */
static int create_keyboard(void) {
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
    snprintf(setup.name, UINPUT_MAX_NAME_SIZE, "RootStream Virtual Keyboard");
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
}

/*
 * Create a virtual mouse device
 */
static int create_mouse(void) {
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

    /* Enable relative axes */
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);
    ioctl(fd, UI_SET_RELBIT, REL_WHEEL);

    /* Setup device */
    struct uinput_setup setup = {0};
    snprintf(setup.name, UINPUT_MAX_NAME_SIZE, "RootStream Virtual Mouse");
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
}

/*
 * Emit an input event
 */
static int emit_event(int fd, uint16_t type, uint16_t code, int32_t value) {
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
}

/*
 * Initialize input system
 */
int rootstream_input_init(rootstream_ctx_t *ctx) {
    if (!ctx) {
        fprintf(stderr, "Invalid context\n");
        return -1;
    }

    /* Create virtual devices */
    ctx->uinput_kbd_fd = create_keyboard();
    if (ctx->uinput_kbd_fd < 0) {
        fprintf(stderr, "Cannot create virtual keyboard: %s\n", strerror(errno));
        return -1;
    }

    ctx->uinput_mouse_fd = create_mouse();
    if (ctx->uinput_mouse_fd < 0) {
        close(ctx->uinput_kbd_fd);
        fprintf(stderr, "Cannot create virtual mouse: %s\n", strerror(errno));
        return -1;
    }

    printf("✓ Virtual input devices created\n");
    return 0;
}

/*
 * Process an input event from network
 */
int rootstream_input_process(rootstream_ctx_t *ctx, input_event_pkt_t *event) {
    if (!ctx || !event) {
        fprintf(stderr, "Invalid arguments\n");
        return -1;
    }

    switch (event->type) {
        case EV_KEY:
            /* Keyboard or mouse button */
            if (event->code < BTN_MOUSE) {
                /* Keyboard */
                return emit_event(ctx->uinput_kbd_fd, EV_KEY, 
                                event->code, event->value);
            } else {
                /* Mouse button */
                return emit_event(ctx->uinput_mouse_fd, EV_KEY,
                                event->code, event->value);
            }
            break;

        case EV_REL:
            /* Mouse movement */
            return emit_event(ctx->uinput_mouse_fd, EV_REL,
                            event->code, event->value);
            break;

        default:
            /* Unsupported event type */
            return 0;
    }

    return 0;
}

/*
 * Cleanup input system
 */
void rootstream_input_cleanup(rootstream_ctx_t *ctx) {
    if (!ctx)
        return;

    if (ctx->uinput_kbd_fd >= 0) {
        ioctl(ctx->uinput_kbd_fd, UI_DEV_DESTROY);
        close(ctx->uinput_kbd_fd);
        ctx->uinput_kbd_fd = -1;
    }

    if (ctx->uinput_mouse_fd >= 0) {
        ioctl(ctx->uinput_mouse_fd, UI_DEV_DESTROY);
        close(ctx->uinput_mouse_fd);
        ctx->uinput_mouse_fd = -1;
    }
}
