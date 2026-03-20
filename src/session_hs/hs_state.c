/*
 * hs_state.c — Session handshake FSM implementation
 */

#include "hs_state.h"

#include <stdlib.h>

struct hs_fsm_s {
    hs_role_t role;
    hs_state_t state;
    uint8_t error_reason;
};

hs_fsm_t *hs_fsm_create(hs_role_t role) {
    hs_fsm_t *fsm = calloc(1, sizeof(*fsm));
    if (!fsm)
        return NULL;
    fsm->role = role;
    fsm->state = HS_ST_INIT;
    return fsm;
}

void hs_fsm_destroy(hs_fsm_t *fsm) {
    free(fsm);
}

hs_state_t hs_fsm_state(const hs_fsm_t *fsm) {
    return fsm ? fsm->state : HS_ST_ERROR;
}

int hs_fsm_process(hs_fsm_t *fsm, const hs_message_t *msg) {
    if (!fsm || !msg)
        return -1;
    if (hs_fsm_is_terminal(fsm))
        return -1;

    hs_state_t next = fsm->state;
    int ok = 0;

    if (fsm->role == HS_ROLE_CLIENT) {
        switch (fsm->state) {
            case HS_ST_INIT:
                /* Client calls hs_fsm_process(HELLO) to mark HELLO_SENT */
                if (msg->type == HS_MSG_HELLO) {
                    next = HS_ST_HELLO_SENT;
                    ok = 1;
                }
                break;
            case HS_ST_HELLO_SENT:
                if (msg->type == HS_MSG_HELLO_ACK) {
                    next = HS_ST_AUTH;
                    ok = 1;
                }
                break;
            case HS_ST_AUTH:
                if (msg->type == HS_MSG_AUTH) {
                    next = HS_ST_AUTH_SENT;
                    ok = 1;
                }
                break;
            case HS_ST_AUTH_SENT:
                if (msg->type == HS_MSG_AUTH_ACK) {
                    next = HS_ST_CONFIG_WAIT;
                    ok = 1;
                }
                break;
            case HS_ST_CONFIG_WAIT:
                if (msg->type == HS_MSG_CONFIG) {
                    next = HS_ST_READY;
                    ok = 1;
                }
                break;
            case HS_ST_READY:
                if (msg->type == HS_MSG_BYE) {
                    next = HS_ST_CLOSED;
                    ok = 1;
                }
                break;
            default:
                break;
        }
    } else { /* SERVER */
        switch (fsm->state) {
            case HS_ST_INIT:
                if (msg->type == HS_MSG_HELLO) {
                    next = HS_ST_HELLO_RCVD;
                    ok = 1;
                }
                break;
            case HS_ST_HELLO_RCVD:
                if (msg->type == HS_MSG_HELLO_ACK) {
                    next = HS_ST_AUTH_WAIT;
                    ok = 1;
                }
                break;
            case HS_ST_AUTH_WAIT:
                if (msg->type == HS_MSG_AUTH) {
                    next = HS_ST_AUTH_VERIFY;
                    ok = 1;
                }
                break;
            case HS_ST_AUTH_VERIFY:
                if (msg->type == HS_MSG_AUTH_ACK) {
                    next = HS_ST_CONFIG_SENT;
                    ok = 1;
                }
                break;
            case HS_ST_CONFIG_SENT:
                if (msg->type == HS_MSG_CONFIG) {
                    next = HS_ST_READY;
                    ok = 1;
                }
                break;
            case HS_ST_READY:
                if (msg->type == HS_MSG_BYE) {
                    next = HS_ST_CLOSED;
                    ok = 1;
                }
                break;
            default:
                break;
        }
    }

    if (msg->type == HS_MSG_ERROR) {
        fsm->state = HS_ST_ERROR;
        return 0;
    }
    if (msg->type == HS_MSG_BYE) {
        fsm->state = HS_ST_CLOSED;
        return 0;
    }

    if (!ok)
        return -1;
    fsm->state = next;
    return 0;
}

void hs_fsm_set_error(hs_fsm_t *fsm, uint8_t reason) {
    if (!fsm)
        return;
    fsm->state = HS_ST_ERROR;
    fsm->error_reason = reason;
}

void hs_fsm_close(hs_fsm_t *fsm) {
    if (fsm)
        fsm->state = HS_ST_CLOSED;
}

bool hs_fsm_is_terminal(const hs_fsm_t *fsm) {
    if (!fsm)
        return true;
    return fsm->state == HS_ST_ERROR || fsm->state == HS_ST_CLOSED;
}

const char *hs_state_name(hs_state_t s) {
    switch (s) {
        case HS_ST_INIT:
            return "INIT";
        case HS_ST_HELLO_SENT:
            return "HELLO_SENT";
        case HS_ST_HELLO_RCVD:
            return "HELLO_RCVD";
        case HS_ST_AUTH:
            return "AUTH";
        case HS_ST_AUTH_WAIT:
            return "AUTH_WAIT";
        case HS_ST_AUTH_SENT:
            return "AUTH_SENT";
        case HS_ST_AUTH_VERIFY:
            return "AUTH_VERIFY";
        case HS_ST_CONFIG_WAIT:
            return "CONFIG_WAIT";
        case HS_ST_CONFIG_SENT:
            return "CONFIG_SENT";
        case HS_ST_READY:
            return "READY";
        case HS_ST_ERROR:
            return "ERROR";
        case HS_ST_CLOSED:
            return "CLOSED";
        default:
            return "UNKNOWN";
    }
}
