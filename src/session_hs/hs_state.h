/*
 * hs_state.h — Session handshake finite-state machine
 *
 * Models both the client-side and server-side view of the handshake.
 * State sequence:
 *
 *   Client                            Server
 *   ──────                            ──────
 *   HS_ST_INIT                        HS_ST_INIT
 *     → send HELLO                      ← recv HELLO
 *   HS_ST_HELLO_SENT               HS_ST_HELLO_RCVD
 *     ← recv HELLO_ACK                  → send HELLO_ACK
 *   HS_ST_AUTH                     HS_ST_AUTH_WAIT
 *     → send AUTH                       ← recv AUTH
 *   HS_ST_AUTH_SENT                HS_ST_AUTH_VERIFY
 *     ← recv AUTH_ACK (ok)              → send AUTH_ACK
 *   HS_ST_CONFIG_WAIT              HS_ST_CONFIG_SENT
 *     ← recv CONFIG                     …(config sent)
 *   HS_ST_READY                    HS_ST_READY
 *
 *   Any state → HS_ST_ERROR on error
 *   Any state → HS_ST_CLOSED on BYE / explicit close
 *
 * Thread-safety: NOT thread-safe.
 */

#ifndef ROOTSTREAM_HS_STATE_H
#define ROOTSTREAM_HS_STATE_H

#include "hs_message.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** FSM states (shared client/server) */
typedef enum {
    HS_ST_INIT        = 0,
    HS_ST_HELLO_SENT  = 1,   /**< Client: HELLO sent, awaiting HELLO_ACK */
    HS_ST_HELLO_RCVD  = 2,   /**< Server: HELLO received */
    HS_ST_AUTH        = 3,   /**< Client: HELLO_ACK received, building AUTH */
    HS_ST_AUTH_WAIT   = 4,   /**< Server: HELLO_ACK sent, awaiting AUTH */
    HS_ST_AUTH_SENT   = 5,   /**< Client: AUTH sent, awaiting AUTH_ACK */
    HS_ST_AUTH_VERIFY = 6,   /**< Server: AUTH received, verifying */
    HS_ST_CONFIG_WAIT = 7,   /**< Client: AUTH_ACK OK, awaiting CONFIG */
    HS_ST_CONFIG_SENT = 8,   /**< Server: CONFIG sent */
    HS_ST_READY       = 9,   /**< Both: stream ready */
    HS_ST_ERROR       = 10,  /**< Terminal: error */
    HS_ST_CLOSED      = 11,  /**< Terminal: graceful close */
} hs_state_t;

/** Role of this FSM instance */
typedef enum {
    HS_ROLE_CLIENT = 0,
    HS_ROLE_SERVER = 1,
} hs_role_t;

/** Opaque FSM context */
typedef struct hs_fsm_s hs_fsm_t;

/**
 * hs_fsm_create — allocate FSM
 *
 * @param role  Client or server role
 * @return      Non-NULL handle, or NULL on OOM
 */
hs_fsm_t *hs_fsm_create(hs_role_t role);

/**
 * hs_fsm_destroy — free FSM
 *
 * @param fsm  FSM to destroy
 */
void hs_fsm_destroy(hs_fsm_t *fsm);

/**
 * hs_fsm_state — current FSM state
 *
 * @param fsm  FSM
 * @return     Current state
 */
hs_state_t hs_fsm_state(const hs_fsm_t *fsm);

/**
 * hs_fsm_process — advance FSM on receipt of @msg
 *
 * @param fsm  FSM
 * @param msg  Incoming message
 * @return     0 on success (state advanced), -1 on unexpected message
 */
int hs_fsm_process(hs_fsm_t *fsm, const hs_message_t *msg);

/**
 * hs_fsm_set_error — transition to HS_ST_ERROR with a reason code
 *
 * @param fsm     FSM
 * @param reason  Error reason byte
 */
void hs_fsm_set_error(hs_fsm_t *fsm, uint8_t reason);

/**
 * hs_fsm_close — transition to HS_ST_CLOSED
 *
 * @param fsm  FSM
 */
void hs_fsm_close(hs_fsm_t *fsm);

/**
 * hs_fsm_is_terminal — return true if state is ERROR or CLOSED
 *
 * @param fsm  FSM
 * @return     true if terminal
 */
bool hs_fsm_is_terminal(const hs_fsm_t *fsm);

/**
 * hs_state_name — human-readable state name
 *
 * @param s  State
 * @return   Static string
 */
const char *hs_state_name(hs_state_t s);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_HS_STATE_H */
