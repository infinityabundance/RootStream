/*
 * relay_client.c — Client-side relay connector implementation
 */

#include "relay_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct relay_client_s {
    relay_io_t io;
    uint8_t token[RELAY_TOKEN_LEN];
    bool is_host;
    relay_client_state_t state;
    relay_session_id_t session_id;
};

relay_client_t *relay_client_create(const relay_io_t *io, const uint8_t *token, bool is_host) {
    if (!io || !io->send_fn || !token)
        return NULL;

    relay_client_t *c = calloc(1, sizeof(*c));
    if (!c)
        return NULL;

    c->io = *io;
    c->is_host = is_host;
    c->state = RELAY_CLIENT_DISCONNECTED;
    memcpy(c->token, token, RELAY_TOKEN_LEN);
    return c;
}

void relay_client_destroy(relay_client_t *client) {
    free(client);
}

relay_client_state_t relay_client_get_state(const relay_client_t *client) {
    return client ? client->state : RELAY_CLIENT_DISCONNECTED;
}

relay_session_id_t relay_client_get_session_id(const relay_client_t *client) {
    return (client && client->state == RELAY_CLIENT_READY) ? client->session_id : 0;
}

int relay_client_connect(relay_client_t *client) {
    if (!client)
        return -1;
    if (client->state != RELAY_CLIENT_DISCONNECTED)
        return -1;

    /* Build HELLO payload */
    uint8_t hello_payload[36];
    relay_build_hello(client->token, client->is_host, hello_payload);

    /* Build full message: header + payload */
    uint8_t msg[RELAY_HDR_SIZE + 36];
    relay_header_t hdr = {
        .type = RELAY_MSG_HELLO,
        .session_id = 0,
        .payload_len = 36,
    };
    relay_encode_header(&hdr, msg);
    memcpy(msg + RELAY_HDR_SIZE, hello_payload, 36);

    int sent = client->io.send_fn(msg, sizeof(msg), client->io.user_data);
    if (sent != (int)sizeof(msg)) {
        client->state = RELAY_CLIENT_ERROR;
        return -1;
    }

    client->state = RELAY_CLIENT_HELLO_SENT;
    return 0;
}

int relay_client_receive(relay_client_t *client, const uint8_t *buf, size_t len,
                         void (*data_cb)(const uint8_t *, size_t, void *), void *data_ud) {
    if (!client || !buf || len < (size_t)RELAY_HDR_SIZE)
        return -1;

    relay_header_t hdr;
    if (relay_decode_header(buf, &hdr) != 0)
        return -1;

    const uint8_t *payload = buf + RELAY_HDR_SIZE;

    switch (hdr.type) {
        case RELAY_MSG_HELLO_ACK:
            if (client->state == RELAY_CLIENT_HELLO_SENT) {
                client->session_id = hdr.session_id;
                client->state = RELAY_CLIENT_READY;
            }
            break;

        case RELAY_MSG_DATA:
            if (data_cb && hdr.payload_len > 0) {
                data_cb(payload, hdr.payload_len, data_ud);
            }
            break;

        case RELAY_MSG_PING: {
            /* Auto-respond with PONG */
            uint8_t pong[RELAY_HDR_SIZE];
            relay_header_t pong_hdr = {
                .type = RELAY_MSG_PONG,
                .session_id = client->session_id,
                .payload_len = 0,
            };
            relay_encode_header(&pong_hdr, pong);
            client->io.send_fn(pong, sizeof(pong), client->io.user_data);
            break;
        }

        case RELAY_MSG_DISCONNECT:
            client->state = RELAY_CLIENT_DISCONNECTED;
            break;

        case RELAY_MSG_ERROR:
            client->state = RELAY_CLIENT_ERROR;
            break;

        default:
            break;
    }

    return 0;
}

int relay_client_send_data(relay_client_t *client, const uint8_t *payload, size_t payload_len) {
    if (!client || !payload)
        return -1;
    if (client->state != RELAY_CLIENT_READY)
        return -1;
    if (payload_len > RELAY_MAX_PAYLOAD)
        return -1;

    /* Build header + payload into a single buffer */
    size_t total = (size_t)RELAY_HDR_SIZE + payload_len;
    uint8_t *msg = malloc(total);
    if (!msg)
        return -1;

    relay_header_t hdr = {
        .type = RELAY_MSG_DATA,
        .session_id = client->session_id,
        .payload_len = (uint16_t)payload_len,
    };
    relay_encode_header(&hdr, msg);
    memcpy(msg + RELAY_HDR_SIZE, payload, payload_len);

    int sent = client->io.send_fn(msg, total, client->io.user_data);
    free(msg);

    return (sent == (int)total) ? 0 : -1;
}
