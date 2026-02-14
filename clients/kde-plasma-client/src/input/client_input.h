/**
 * @file client_input.h
 * @brief Client-side input capture for RootStream
 * 
 * Captures keyboard, mouse, and gamepad input from the client and
 * prepares it for transmission to the host.
 */

#ifndef CLIENT_INPUT_H
#define CLIENT_INPUT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Input event types (matching Linux input event types)
 */
#define EV_SYN          0x00
#define EV_KEY          0x01
#define EV_REL          0x02
#define EV_ABS          0x03

/**
 * Mouse button codes
 */
#define BTN_LEFT        0x110
#define BTN_RIGHT       0x111
#define BTN_MIDDLE      0x112

/**
 * Relative axes codes
 */
#define REL_X           0x00
#define REL_Y           0x01
#define REL_WHEEL       0x08

/**
 * Input event structure (matches network protocol)
 */
typedef struct {
    uint8_t type;              /* EV_KEY, EV_REL, etc */
    uint16_t code;             /* Key/button code */
    int32_t value;             /* Value/delta */
    uint64_t timestamp_us;     /* Timestamp in microseconds */
} client_input_event_t;

/**
 * Input capture callback
 * Called when an input event is captured
 */
typedef void (*input_event_callback_t)(const client_input_event_t *event, void *user_data);

/**
 * Input capture context
 */
typedef struct client_input_ctx client_input_ctx_t;

/**
 * Initialize input capture
 * 
 * @param callback Callback function for captured events
 * @param user_data User data passed to callback
 * @return Input context, or NULL on failure
 */
client_input_ctx_t* client_input_init(input_event_callback_t callback, void *user_data);

/**
 * Start capturing input
 * 
 * @param ctx Input context
 * @param native_window Native window handle (X11 Window, etc.)
 * @return 0 on success, -1 on failure
 */
int client_input_start_capture(client_input_ctx_t *ctx, void *native_window);

/**
 * Stop capturing input
 * 
 * @param ctx Input context
 */
void client_input_stop_capture(client_input_ctx_t *ctx);

/**
 * Process pending input events
 * Should be called regularly from main loop
 * 
 * @param ctx Input context
 * @return Number of events processed
 */
int client_input_process_events(client_input_ctx_t *ctx);

/**
 * Enable/disable mouse capture mode
 * When enabled, mouse is grabbed and confined to window
 * 
 * @param ctx Input context
 * @param enable True to enable, false to disable
 * @return 0 on success, -1 on failure
 */
int client_input_set_mouse_capture(client_input_ctx_t *ctx, bool enable);

/**
 * Get current mouse capture state
 * 
 * @param ctx Input context
 * @return True if mouse is captured, false otherwise
 */
bool client_input_is_mouse_captured(client_input_ctx_t *ctx);

/**
 * Clean up and destroy input context
 * 
 * @param ctx Input context
 */
void client_input_cleanup(client_input_ctx_t *ctx);

/**
 * Get last error message
 * 
 * @param ctx Input context
 * @return Error string, or NULL if no error
 */
const char* client_input_get_error(client_input_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* CLIENT_INPUT_H */
