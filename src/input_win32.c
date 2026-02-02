/*
 * input_win32.c - Windows SendInput-based input injection
 *
 * Injects keyboard and mouse input from the remote client using
 * the Windows SendInput API. Works with any application.
 *
 * Converts Linux input event codes (from the network protocol)
 * to Windows virtual key codes and mouse events.
 */

#ifdef _WIN32

#include "../include/rootstream.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>

/* Linux input event types (from linux/input-event-codes.h) */
#define EV_SYN 0x00
#define EV_KEY 0x01
#define EV_REL 0x02

/* Linux relative axis codes */
#define REL_X    0x00
#define REL_Y    0x01
#define REL_WHEEL 0x08
#define REL_HWHEEL 0x06

/* Linux mouse button codes */
#define BTN_MOUSE   0x110
#define BTN_LEFT    0x110
#define BTN_RIGHT   0x111
#define BTN_MIDDLE  0x112
#define BTN_SIDE    0x113
#define BTN_EXTRA   0x114

/* ============================================================================
 * Linux to Windows Keycode Mapping
 * ============================================================================
 *
 * Linux keycodes are defined in linux/input-event-codes.h
 * Windows virtual key codes are defined in winuser.h
 */

/* Linux keyboard codes (from input-event-codes.h) */
#define KEY_ESC         1
#define KEY_1           2
#define KEY_2           3
#define KEY_3           4
#define KEY_4           5
#define KEY_5           6
#define KEY_6           7
#define KEY_7           8
#define KEY_8           9
#define KEY_9           10
#define KEY_0           11
#define KEY_MINUS       12
#define KEY_EQUAL       13
#define KEY_BACKSPACE   14
#define KEY_TAB         15
#define KEY_Q           16
#define KEY_W           17
#define KEY_E           18
#define KEY_R           19
#define KEY_T           20
#define KEY_Y           21
#define KEY_U           22
#define KEY_I           23
#define KEY_O           24
#define KEY_P           25
#define KEY_LEFTBRACE   26
#define KEY_RIGHTBRACE  27
#define KEY_ENTER       28
#define KEY_LEFTCTRL    29
#define KEY_A           30
#define KEY_S           31
#define KEY_D           32
#define KEY_F           33
#define KEY_G           34
#define KEY_H           35
#define KEY_J           36
#define KEY_K           37
#define KEY_L           38
#define KEY_SEMICOLON   39
#define KEY_APOSTROPHE  40
#define KEY_GRAVE       41
#define KEY_LEFTSHIFT   42
#define KEY_BACKSLASH   43
#define KEY_Z           44
#define KEY_X           45
#define KEY_C           46
#define KEY_V           47
#define KEY_B           48
#define KEY_N           49
#define KEY_M           50
#define KEY_COMMA       51
#define KEY_DOT         52
#define KEY_SLASH       53
#define KEY_RIGHTSHIFT  54
#define KEY_KPASTERISK  55
#define KEY_LEFTALT     56
#define KEY_SPACE       57
#define KEY_CAPSLOCK    58
#define KEY_F1          59
#define KEY_F2          60
#define KEY_F3          61
#define KEY_F4          62
#define KEY_F5          63
#define KEY_F6          64
#define KEY_F7          65
#define KEY_F8          66
#define KEY_F9          67
#define KEY_F10         68
#define KEY_NUMLOCK     69
#define KEY_SCROLLLOCK  70
#define KEY_KP7         71
#define KEY_KP8         72
#define KEY_KP9         73
#define KEY_KPMINUS     74
#define KEY_KP4         75
#define KEY_KP5         76
#define KEY_KP6         77
#define KEY_KPPLUS      78
#define KEY_KP1         79
#define KEY_KP2         80
#define KEY_KP3         81
#define KEY_KP0         82
#define KEY_KPDOT       83
#define KEY_F11         87
#define KEY_F12         88
#define KEY_KPENTER     96
#define KEY_RIGHTCTRL   97
#define KEY_KPSLASH     98
#define KEY_SYSRQ       99
#define KEY_RIGHTALT    100
#define KEY_HOME        102
#define KEY_UP          103
#define KEY_PAGEUP      104
#define KEY_LEFT        105
#define KEY_RIGHT       106
#define KEY_END         107
#define KEY_DOWN        108
#define KEY_PAGEDOWN    109
#define KEY_INSERT      110
#define KEY_DELETE      111
#define KEY_PAUSE       119
#define KEY_LEFTMETA    125
#define KEY_RIGHTMETA   126
#define KEY_COMPOSE     127

/* Maximum Linux keycode we handle */
#define MAX_LINUX_KEYCODE 256

/* Linux keycode to Windows VK mapping table */
static WORD linux_to_vk[MAX_LINUX_KEYCODE] = {0};
static bool keymap_initialized = false;

static void init_keymap(void) {
    if (keymap_initialized) return;

    /* Clear table */
    memset(linux_to_vk, 0, sizeof(linux_to_vk));

    /* Numbers */
    linux_to_vk[KEY_1] = '1';
    linux_to_vk[KEY_2] = '2';
    linux_to_vk[KEY_3] = '3';
    linux_to_vk[KEY_4] = '4';
    linux_to_vk[KEY_5] = '5';
    linux_to_vk[KEY_6] = '6';
    linux_to_vk[KEY_7] = '7';
    linux_to_vk[KEY_8] = '8';
    linux_to_vk[KEY_9] = '9';
    linux_to_vk[KEY_0] = '0';

    /* Letters */
    linux_to_vk[KEY_A] = 'A';
    linux_to_vk[KEY_B] = 'B';
    linux_to_vk[KEY_C] = 'C';
    linux_to_vk[KEY_D] = 'D';
    linux_to_vk[KEY_E] = 'E';
    linux_to_vk[KEY_F] = 'F';
    linux_to_vk[KEY_G] = 'G';
    linux_to_vk[KEY_H] = 'H';
    linux_to_vk[KEY_I] = 'I';
    linux_to_vk[KEY_J] = 'J';
    linux_to_vk[KEY_K] = 'K';
    linux_to_vk[KEY_L] = 'L';
    linux_to_vk[KEY_M] = 'M';
    linux_to_vk[KEY_N] = 'N';
    linux_to_vk[KEY_O] = 'O';
    linux_to_vk[KEY_P] = 'P';
    linux_to_vk[KEY_Q] = 'Q';
    linux_to_vk[KEY_R] = 'R';
    linux_to_vk[KEY_S] = 'S';
    linux_to_vk[KEY_T] = 'T';
    linux_to_vk[KEY_U] = 'U';
    linux_to_vk[KEY_V] = 'V';
    linux_to_vk[KEY_W] = 'W';
    linux_to_vk[KEY_X] = 'X';
    linux_to_vk[KEY_Y] = 'Y';
    linux_to_vk[KEY_Z] = 'Z';

    /* Function keys */
    linux_to_vk[KEY_F1] = VK_F1;
    linux_to_vk[KEY_F2] = VK_F2;
    linux_to_vk[KEY_F3] = VK_F3;
    linux_to_vk[KEY_F4] = VK_F4;
    linux_to_vk[KEY_F5] = VK_F5;
    linux_to_vk[KEY_F6] = VK_F6;
    linux_to_vk[KEY_F7] = VK_F7;
    linux_to_vk[KEY_F8] = VK_F8;
    linux_to_vk[KEY_F9] = VK_F9;
    linux_to_vk[KEY_F10] = VK_F10;
    linux_to_vk[KEY_F11] = VK_F11;
    linux_to_vk[KEY_F12] = VK_F12;

    /* Modifiers */
    linux_to_vk[KEY_LEFTSHIFT] = VK_LSHIFT;
    linux_to_vk[KEY_RIGHTSHIFT] = VK_RSHIFT;
    linux_to_vk[KEY_LEFTCTRL] = VK_LCONTROL;
    linux_to_vk[KEY_RIGHTCTRL] = VK_RCONTROL;
    linux_to_vk[KEY_LEFTALT] = VK_LMENU;
    linux_to_vk[KEY_RIGHTALT] = VK_RMENU;
    linux_to_vk[KEY_LEFTMETA] = VK_LWIN;
    linux_to_vk[KEY_RIGHTMETA] = VK_RWIN;

    /* Special keys */
    linux_to_vk[KEY_ESC] = VK_ESCAPE;
    linux_to_vk[KEY_TAB] = VK_TAB;
    linux_to_vk[KEY_CAPSLOCK] = VK_CAPITAL;
    linux_to_vk[KEY_ENTER] = VK_RETURN;
    linux_to_vk[KEY_BACKSPACE] = VK_BACK;
    linux_to_vk[KEY_SPACE] = VK_SPACE;

    /* Navigation */
    linux_to_vk[KEY_INSERT] = VK_INSERT;
    linux_to_vk[KEY_DELETE] = VK_DELETE;
    linux_to_vk[KEY_HOME] = VK_HOME;
    linux_to_vk[KEY_END] = VK_END;
    linux_to_vk[KEY_PAGEUP] = VK_PRIOR;
    linux_to_vk[KEY_PAGEDOWN] = VK_NEXT;
    linux_to_vk[KEY_UP] = VK_UP;
    linux_to_vk[KEY_DOWN] = VK_DOWN;
    linux_to_vk[KEY_LEFT] = VK_LEFT;
    linux_to_vk[KEY_RIGHT] = VK_RIGHT;

    /* Punctuation */
    linux_to_vk[KEY_MINUS] = VK_OEM_MINUS;
    linux_to_vk[KEY_EQUAL] = VK_OEM_PLUS;
    linux_to_vk[KEY_LEFTBRACE] = VK_OEM_4;
    linux_to_vk[KEY_RIGHTBRACE] = VK_OEM_6;
    linux_to_vk[KEY_SEMICOLON] = VK_OEM_1;
    linux_to_vk[KEY_APOSTROPHE] = VK_OEM_7;
    linux_to_vk[KEY_GRAVE] = VK_OEM_3;
    linux_to_vk[KEY_BACKSLASH] = VK_OEM_5;
    linux_to_vk[KEY_COMMA] = VK_OEM_COMMA;
    linux_to_vk[KEY_DOT] = VK_OEM_PERIOD;
    linux_to_vk[KEY_SLASH] = VK_OEM_2;

    /* Numpad */
    linux_to_vk[KEY_NUMLOCK] = VK_NUMLOCK;
    linux_to_vk[KEY_KP0] = VK_NUMPAD0;
    linux_to_vk[KEY_KP1] = VK_NUMPAD1;
    linux_to_vk[KEY_KP2] = VK_NUMPAD2;
    linux_to_vk[KEY_KP3] = VK_NUMPAD3;
    linux_to_vk[KEY_KP4] = VK_NUMPAD4;
    linux_to_vk[KEY_KP5] = VK_NUMPAD5;
    linux_to_vk[KEY_KP6] = VK_NUMPAD6;
    linux_to_vk[KEY_KP7] = VK_NUMPAD7;
    linux_to_vk[KEY_KP8] = VK_NUMPAD8;
    linux_to_vk[KEY_KP9] = VK_NUMPAD9;
    linux_to_vk[KEY_KPASTERISK] = VK_MULTIPLY;
    linux_to_vk[KEY_KPMINUS] = VK_SUBTRACT;
    linux_to_vk[KEY_KPPLUS] = VK_ADD;
    linux_to_vk[KEY_KPDOT] = VK_DECIMAL;
    linux_to_vk[KEY_KPSLASH] = VK_DIVIDE;
    linux_to_vk[KEY_KPENTER] = VK_RETURN;  /* Same as regular enter */

    /* Lock keys */
    linux_to_vk[KEY_SCROLLLOCK] = VK_SCROLL;
    linux_to_vk[KEY_PAUSE] = VK_PAUSE;
    linux_to_vk[KEY_SYSRQ] = VK_SNAPSHOT;  /* Print Screen */

    keymap_initialized = true;
}

/* ============================================================================
 * Input Injection
 * ============================================================================ */

/*
 * Inject a keyboard event
 */
static int inject_key(uint16_t linux_code, int32_t value) {
    if (linux_code >= MAX_LINUX_KEYCODE) {
        return 0;  /* Unknown key, ignore */
    }

    WORD vk = linux_to_vk[linux_code];
    if (vk == 0) {
        return 0;  /* No mapping, ignore */
    }

    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.wScan = (WORD)MapVirtualKey(vk, MAPVK_VK_TO_VSC);

    /* value: 1 = press, 0 = release, 2 = repeat (treat as press) */
    if (value == 0) {
        input.ki.dwFlags = KEYEVENTF_KEYUP;
    } else {
        input.ki.dwFlags = 0;  /* Key down */
    }

    /* Extended key flag for certain keys */
    if (vk == VK_RCONTROL || vk == VK_RMENU ||
        vk == VK_INSERT || vk == VK_DELETE ||
        vk == VK_HOME || vk == VK_END ||
        vk == VK_PRIOR || vk == VK_NEXT ||
        vk == VK_UP || vk == VK_DOWN ||
        vk == VK_LEFT || vk == VK_RIGHT ||
        vk == VK_LWIN || vk == VK_RWIN ||
        vk == VK_DIVIDE || vk == VK_NUMLOCK) {
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }

    if (SendInput(1, &input, sizeof(INPUT)) != 1) {
        return -1;
    }

    return 0;
}

/*
 * Inject a mouse button event
 */
static int inject_mouse_button(uint16_t button, int32_t value) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;

    switch (button) {
        case BTN_LEFT:
            input.mi.dwFlags = value ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
            break;
        case BTN_RIGHT:
            input.mi.dwFlags = value ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
            break;
        case BTN_MIDDLE:
            input.mi.dwFlags = value ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
            break;
        case BTN_SIDE:
            input.mi.dwFlags = value ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
            input.mi.mouseData = XBUTTON1;
            break;
        case BTN_EXTRA:
            input.mi.dwFlags = value ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
            input.mi.mouseData = XBUTTON2;
            break;
        default:
            return 0;  /* Unknown button */
    }

    if (SendInput(1, &input, sizeof(INPUT)) != 1) {
        return -1;
    }

    return 0;
}

/*
 * Inject a mouse movement event (relative)
 */
static int inject_mouse_move(uint16_t axis, int32_t delta) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;

    switch (axis) {
        case REL_X:
            input.mi.dx = delta;
            input.mi.dy = 0;
            break;
        case REL_Y:
            input.mi.dx = 0;
            input.mi.dy = delta;
            break;
        case REL_WHEEL:
            input.mi.dwFlags = MOUSEEVENTF_WHEEL;
            input.mi.mouseData = delta * WHEEL_DELTA;
            break;
        case REL_HWHEEL:
            input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
            input.mi.mouseData = delta * WHEEL_DELTA;
            break;
        default:
            return 0;  /* Unknown axis */
    }

    if (SendInput(1, &input, sizeof(INPUT)) != 1) {
        return -1;
    }

    return 0;
}

/* ============================================================================
 * Public API
 * ============================================================================ */

/*
 * Initialize input system
 *
 * On Windows, we don't need to create virtual devices - SendInput works
 * directly with the system. We just initialize the keymap.
 */
int rootstream_input_init(rootstream_ctx_t *ctx) {
    if (!ctx) {
        fprintf(stderr, "Invalid context\n");
        return -1;
    }

    /* Initialize keycode mapping */
    init_keymap();

    /* Mark as initialized (use -1 to indicate "not using uinput") */
    ctx->uinput_kbd_fd = -1;
    ctx->uinput_mouse_fd = -1;

    printf("✓ Input injection ready (Windows SendInput)\n");
    return 0;
}

/*
 * Process an input event from network
 *
 * The event structure uses Linux input codes which we convert to
 * Windows equivalents before injecting.
 */
int rootstream_input_process(rootstream_ctx_t *ctx, input_event_pkt_t *event) {
    if (!ctx || !event) {
        return -1;
    }

    switch (event->type) {
        case EV_KEY:
            /* Keyboard or mouse button */
            if (event->code < BTN_MOUSE) {
                /* Keyboard key */
                return inject_key(event->code, event->value);
            } else {
                /* Mouse button */
                return inject_mouse_button(event->code, event->value);
            }

        case EV_REL:
            /* Mouse movement or scroll */
            return inject_mouse_move(event->code, event->value);

        case EV_SYN:
            /* Sync events - no action needed on Windows */
            return 0;

        default:
            /* Unknown event type */
            return 0;
    }
}

/*
 * Cleanup input system
 *
 * Nothing to clean up on Windows - SendInput doesn't require
 * any persistent state.
 */
void rootstream_input_cleanup(rootstream_ctx_t *ctx) {
    if (!ctx) {
        return;
    }

    /* Reset file descriptors (even though unused) */
    ctx->uinput_kbd_fd = -1;
    ctx->uinput_mouse_fd = -1;

    printf("✓ Input injection cleanup complete (Windows)\n");
}

#endif /* _WIN32 */
