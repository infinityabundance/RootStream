/*
 * key_exchange.h - ECDH and X3DH key exchange for RootStream
 * 
 * Provides secure key agreement using Curve25519 (X25519)
 * Implements X3DH protocol for asynchronous key exchange
 */

#ifndef ROOTSTREAM_KEY_EXCHANGE_H
#define ROOTSTREAM_KEY_EXCHANGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Key sizes */
#define KEY_EXCHANGE_PUBLIC_KEY_BYTES 32
#define KEY_EXCHANGE_SECRET_KEY_BYTES 32
#define KEY_EXCHANGE_SHARED_SECRET_BYTES 32
#define KEY_EXCHANGE_SIGNATURE_BYTES 64

/* Keypair structure */
typedef struct {
    uint8_t public_key[KEY_EXCHANGE_PUBLIC_KEY_BYTES];
    uint8_t secret_key[KEY_EXCHANGE_SECRET_KEY_BYTES];
} key_exchange_keypair_t;

/* X3DH key bundle for recipient */
typedef struct {
    uint8_t identity_key[KEY_EXCHANGE_PUBLIC_KEY_BYTES];
    uint8_t signed_prekey[KEY_EXCHANGE_PUBLIC_KEY_BYTES];
    uint8_t signature[KEY_EXCHANGE_SIGNATURE_BYTES];
    uint32_t prekey_id;
} key_exchange_x3dh_bundle_t;

/* X3DH initial message from sender */
typedef struct {
    uint8_t ephemeral_key[KEY_EXCHANGE_PUBLIC_KEY_BYTES];
    uint32_t prekey_id;
} key_exchange_x3dh_init_t;

/*
 * Generate a new keypair for key exchange
 * 
 * @param keypair Output keypair structure
 * @return        0 on success, -1 on error
 */
int key_exchange_generate_keypair(key_exchange_keypair_t *keypair);

/*
 * Compute shared secret using ECDH
 * 
 * @param secret_key       Our secret key (32 bytes)
 * @param peer_public_key  Peer's public key (32 bytes)
 * @param shared_secret    Output shared secret (32 bytes)
 * @return                 0 on success, -1 on error
 */
int key_exchange_compute_shared_secret(
    const uint8_t *secret_key,
    const uint8_t *peer_public_key,
    uint8_t *shared_secret);

/*
 * Create X3DH key bundle (recipient prepares for key exchange)
 * 
 * @param identity_keypair Long-term identity keypair
 * @param bundle           Output bundle structure
 * @return                 0 on success, -1 on error
 */
int key_exchange_x3dh_create_bundle(
    const key_exchange_keypair_t *identity_keypair,
    key_exchange_x3dh_bundle_t *bundle);

/*
 * X3DH initiator (sender computes shared secret)
 * 
 * @param recipient_bundle Recipient's key bundle
 * @param init_msg         Output initial message to send
 * @param shared_secret    Output shared secret (32 bytes)
 * @return                 0 on success, -1 on error
 */
int key_exchange_x3dh_initiator(
    const key_exchange_x3dh_bundle_t *recipient_bundle,
    key_exchange_x3dh_init_t *init_msg,
    uint8_t *shared_secret);

/*
 * X3DH responder (recipient computes shared secret)
 * 
 * @param init_msg         Initial message from sender
 * @param identity_keypair Our identity keypair
 * @param signed_prekey    Our signed prekey
 * @param shared_secret    Output shared secret (32 bytes)
 * @return                 0 on success, -1 on error
 */
int key_exchange_x3dh_responder(
    const key_exchange_x3dh_init_t *init_msg,
    const key_exchange_keypair_t *identity_keypair,
    const key_exchange_keypair_t *signed_prekey,
    uint8_t *shared_secret);

/*
 * Derive session keys from shared secret (for bidirectional communication)
 * 
 * @param shared_secret         Input shared secret (32 bytes)
 * @param client_to_server_key  Output key for client->server (32 bytes)
 * @param server_to_client_key  Output key for server->client (32 bytes)
 * @param client_nonce          Output nonce base for client (12 bytes)
 * @param server_nonce          Output nonce base for server (12 bytes)
 * @return                      0 on success, -1 on error
 */
int key_exchange_derive_session_keys(
    const uint8_t *shared_secret,
    uint8_t *client_to_server_key,
    uint8_t *server_to_client_key,
    uint8_t *client_nonce,
    uint8_t *server_nonce);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_KEY_EXCHANGE_H */
