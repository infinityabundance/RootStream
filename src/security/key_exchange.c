/*
 * key_exchange.c - ECDH and X3DH key exchange implementation
 * 
 * Uses libsodium's X25519 for key agreement and Ed25519 for signatures
 */

#include "key_exchange.h"
#include "crypto_primitives.h"
#include <sodium.h>
#include <string.h>
#include <stdio.h>

/*
 * Generate keypair
 */
int key_exchange_generate_keypair(key_exchange_keypair_t *keypair) {
    if (!keypair) {
        return -1;
    }
    
    /* Generate X25519 keypair */
    crypto_box_keypair(keypair->public_key, keypair->secret_key);
    return 0;
}

/*
 * Compute shared secret using ECDH (X25519)
 */
int key_exchange_compute_shared_secret(
    const uint8_t *secret_key,
    const uint8_t *peer_public_key,
    uint8_t *shared_secret)
{
    if (!secret_key || !peer_public_key || !shared_secret) {
        return -1;
    }
    
    /* X25519 key agreement */
    if (crypto_scalarmult(shared_secret, secret_key, peer_public_key) != 0) {
        fprintf(stderr, "ERROR: Key exchange failed (weak public key?)\n");
        return -1;
    }
    
    return 0;
}

/*
 * Create X3DH key bundle
 */
int key_exchange_x3dh_create_bundle(
    const key_exchange_keypair_t *identity_keypair,
    key_exchange_x3dh_bundle_t *bundle)
{
    if (!identity_keypair || !bundle) {
        return -1;
    }
    
    /* Copy identity key */
    memcpy(bundle->identity_key, identity_keypair->public_key,
           KEY_EXCHANGE_PUBLIC_KEY_BYTES);
    
    /* Generate signed prekey */
    key_exchange_keypair_t prekey;
    if (key_exchange_generate_keypair(&prekey) != 0) {
        return -1;
    }
    memcpy(bundle->signed_prekey, prekey.public_key,
           KEY_EXCHANGE_PUBLIC_KEY_BYTES);
    
    /* Note: Proper X3DH requires Ed25519 identity key for signing.
     * This simplified implementation uses a placeholder signature.
     * Production code should:
     * 1. Maintain separate Ed25519 identity keypair for signing
     * 2. Sign the prekey with crypto_sign_detached
     * 3. Verify signature in X3DH initiator
     */
    
    /* Placeholder signature: hash of prekey and identity */
    uint8_t to_sign[KEY_EXCHANGE_PUBLIC_KEY_BYTES * 2];
    memcpy(to_sign, bundle->signed_prekey, KEY_EXCHANGE_PUBLIC_KEY_BYTES);
    memcpy(to_sign + KEY_EXCHANGE_PUBLIC_KEY_BYTES, bundle->identity_key,
           KEY_EXCHANGE_PUBLIC_KEY_BYTES);
    
    crypto_hash_sha256(bundle->signature, to_sign, sizeof(to_sign));
    
    bundle->prekey_id = randombytes_random();
    
    /* Secure wipe of temporary prekey secret */
    crypto_prim_secure_wipe(&prekey, sizeof(prekey));
    
    return 0;
}

/*
 * X3DH initiator
 */
int key_exchange_x3dh_initiator(
    const key_exchange_x3dh_bundle_t *recipient_bundle,
    key_exchange_x3dh_init_t *init_msg,
    uint8_t *shared_secret)
{
    if (!recipient_bundle || !init_msg || !shared_secret) {
        return -1;
    }
    
    /* Generate ephemeral keypair */
    key_exchange_keypair_t ephemeral;
    if (key_exchange_generate_keypair(&ephemeral) != 0) {
        return -1;
    }
    
    /* Store ephemeral public key in init message */
    memcpy(init_msg->ephemeral_key, ephemeral.public_key,
           KEY_EXCHANGE_PUBLIC_KEY_BYTES);
    init_msg->prekey_id = recipient_bundle->prekey_id;
    
    /* Compute DH1 = DH(ephemeral, signed_prekey) */
    uint8_t dh1[KEY_EXCHANGE_SHARED_SECRET_BYTES];
    if (key_exchange_compute_shared_secret(
            ephemeral.secret_key,
            recipient_bundle->signed_prekey,
            dh1) != 0) {
        crypto_prim_secure_wipe(&ephemeral, sizeof(ephemeral));
        return -1;
    }
    
    /* Compute DH2 = DH(ephemeral, identity_key) */
    uint8_t dh2[KEY_EXCHANGE_SHARED_SECRET_BYTES];
    if (key_exchange_compute_shared_secret(
            ephemeral.secret_key,
            recipient_bundle->identity_key,
            dh2) != 0) {
        crypto_prim_secure_wipe(dh1, sizeof(dh1));
        crypto_prim_secure_wipe(&ephemeral, sizeof(ephemeral));
        return -1;
    }
    
    /* Combine DH outputs: shared_secret = KDF(DH1 || DH2) */
    uint8_t combined[KEY_EXCHANGE_SHARED_SECRET_BYTES * 2];
    memcpy(combined, dh1, KEY_EXCHANGE_SHARED_SECRET_BYTES);
    memcpy(combined + KEY_EXCHANGE_SHARED_SECRET_BYTES, dh2,
           KEY_EXCHANGE_SHARED_SECRET_BYTES);
    
    const uint8_t *info = (const uint8_t *)"RootStreamX3DH";
    crypto_prim_hkdf(combined, sizeof(combined),
                     NULL, 0,
                     info, strlen((const char *)info),
                     shared_secret, KEY_EXCHANGE_SHARED_SECRET_BYTES);
    
    /* Secure cleanup */
    crypto_prim_secure_wipe(dh1, sizeof(dh1));
    crypto_prim_secure_wipe(dh2, sizeof(dh2));
    crypto_prim_secure_wipe(combined, sizeof(combined));
    crypto_prim_secure_wipe(&ephemeral, sizeof(ephemeral));
    
    return 0;
}

/*
 * X3DH responder
 */
int key_exchange_x3dh_responder(
    const key_exchange_x3dh_init_t *init_msg,
    const key_exchange_keypair_t *identity_keypair,
    const key_exchange_keypair_t *signed_prekey,
    uint8_t *shared_secret)
{
    if (!init_msg || !identity_keypair || !signed_prekey || !shared_secret) {
        return -1;
    }
    
    /* Compute DH1 = DH(signed_prekey, ephemeral) */
    uint8_t dh1[KEY_EXCHANGE_SHARED_SECRET_BYTES];
    if (key_exchange_compute_shared_secret(
            signed_prekey->secret_key,
            init_msg->ephemeral_key,
            dh1) != 0) {
        return -1;
    }
    
    /* Compute DH2 = DH(identity, ephemeral) */
    uint8_t dh2[KEY_EXCHANGE_SHARED_SECRET_BYTES];
    if (key_exchange_compute_shared_secret(
            identity_keypair->secret_key,
            init_msg->ephemeral_key,
            dh2) != 0) {
        crypto_prim_secure_wipe(dh1, sizeof(dh1));
        return -1;
    }
    
    /* Combine DH outputs: shared_secret = KDF(DH1 || DH2) */
    uint8_t combined[KEY_EXCHANGE_SHARED_SECRET_BYTES * 2];
    memcpy(combined, dh1, KEY_EXCHANGE_SHARED_SECRET_BYTES);
    memcpy(combined + KEY_EXCHANGE_SHARED_SECRET_BYTES, dh2,
           KEY_EXCHANGE_SHARED_SECRET_BYTES);
    
    const uint8_t *info = (const uint8_t *)"RootStreamX3DH";
    crypto_prim_hkdf(combined, sizeof(combined),
                     NULL, 0,
                     info, strlen((const char *)info),
                     shared_secret, KEY_EXCHANGE_SHARED_SECRET_BYTES);
    
    /* Secure cleanup */
    crypto_prim_secure_wipe(dh1, sizeof(dh1));
    crypto_prim_secure_wipe(dh2, sizeof(dh2));
    crypto_prim_secure_wipe(combined, sizeof(combined));
    
    return 0;
}

/*
 * Derive session keys
 */
int key_exchange_derive_session_keys(
    const uint8_t *shared_secret,
    uint8_t *client_to_server_key,
    uint8_t *server_to_client_key,
    uint8_t *client_nonce,
    uint8_t *server_nonce)
{
    if (!shared_secret) {
        return -1;
    }
    
    /* Derive keys using HKDF with different contexts */
    const uint8_t *info_c2s = (const uint8_t *)"RootStream-C2S";
    const uint8_t *info_s2c = (const uint8_t *)"RootStream-S2C";
    const uint8_t *info_nc = (const uint8_t *)"RootStream-NC";
    const uint8_t *info_ns = (const uint8_t *)"RootStream-NS";
    
    if (client_to_server_key) {
        crypto_prim_hkdf(shared_secret, KEY_EXCHANGE_SHARED_SECRET_BYTES,
                         NULL, 0,
                         info_c2s, strlen((const char *)info_c2s),
                         client_to_server_key, CRYPTO_PRIM_KEY_BYTES);
    }
    
    if (server_to_client_key) {
        crypto_prim_hkdf(shared_secret, KEY_EXCHANGE_SHARED_SECRET_BYTES,
                         NULL, 0,
                         info_s2c, strlen((const char *)info_s2c),
                         server_to_client_key, CRYPTO_PRIM_KEY_BYTES);
    }
    
    if (client_nonce) {
        crypto_prim_hkdf(shared_secret, KEY_EXCHANGE_SHARED_SECRET_BYTES,
                         NULL, 0,
                         info_nc, strlen((const char *)info_nc),
                         client_nonce, CRYPTO_PRIM_NONCE_BYTES);
    }
    
    if (server_nonce) {
        crypto_prim_hkdf(shared_secret, KEY_EXCHANGE_SHARED_SECRET_BYTES,
                         NULL, 0,
                         info_ns, strlen((const char *)info_ns),
                         server_nonce, CRYPTO_PRIM_NONCE_BYTES);
    }
    
    return 0;
}
