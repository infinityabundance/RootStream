/*
 * fc_params.c — Flow controller parameter block
 */

#include "fc_params.h"

int fc_params_init(fc_params_t *p,
                   uint32_t window_bytes,
                   uint32_t send_budget,
                   uint32_t recv_window,
                   uint32_t credit_step) {
    if (!p || window_bytes == 0 || send_budget == 0 ||
        recv_window == 0 || credit_step == 0) return -1;
    p->window_bytes = window_bytes;
    p->send_budget  = send_budget;
    p->recv_window  = recv_window;
    p->credit_step  = credit_step;
    return 0;
}
