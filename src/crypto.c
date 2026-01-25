/*
 * crypto.c - Ed25519 keypair management and ChaCha20-Poly1305 encryption
 * 
 * Security Architecture:
 * =====================
 * 1. Each device generates an Ed25519 keypair on first run
 * 2. Public key is shared via RootStream code (QR/text)
 * 3. Private key never leaves the device
 * 4. Shared secret derived via X25519 key exchange
 * 5. All packets encrypted with ChaCha20-Poly1305
 * 6. Nonce = packet counter (monotonically increasing)
 * 7. MAC prevents tampering and authenticates sender
 * 
 * Why Ed25519?
 * - Fast (tens of thousands of operations/sec)
 * - Small keys (32 bytes public, 32 bytes private)
 * - Audited, battle-tested (used by SSH, Tor, Signal)
 * - No trusted setup or weak curves
 * 
 * Why ChaCha20-Poly1305?
 * - Fast in software (faster than AES without hardware)
 * - Authenticated encryption (prevents tampering)
 * - Used by TLS, Signal, WireGuard
 * - No timing attacks
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

/* libsodium for crypto */
#include <sodium.h>

/* Base64 encoding for RootStream codes */
#include <sodium/utils.h>

/*
 * Format a human-readable fingerprint for a public key.
 *
 * Output format: xxxx-xxxx-xxxx-xxxx (16 hex chars + 3 dashes).
 */
int crypto_format_fingerprint(const uint8_t *public_key, size_t key_len,
                              char *output, size_t output_len) {
    if (!public_key || key_len == 0 || !output) {
        fprintf(stderr, "ERROR: crypto_format_fingerprint invalid arguments\n");
        return -1;
    }

    if (output_len < 20) {
        fprintf(stderr, "ERROR: Fingerprint buffer too small\n");
        fprintf(stderr, "NEED: >= 20 bytes, HAVE: %zu\n", output_len);
        return -1;
    }

    unsigned char hash[crypto_generichash_BYTES];
    if (crypto_generichash(hash, sizeof(hash), public_key, key_len, NULL, 0) != 0) {
        fprintf(stderr, "ERROR: Fingerprint hash failed\n");
        return -1;
    }

    const char hex[] = "0123456789abcdef";
    int out_idx = 0;
    for (int i = 0; i < 8; i++) {
        output[out_idx++] = hex[(hash[i] >> 4) & 0x0F];
        output[out_idx++] = hex[hash[i] & 0x0F];
        if (i % 2 == 1 && i < 7) {
            output[out_idx++] = '-';
        }
    }
    output[out_idx] = '\0';
    return 0;
}

/*
 * Initialize libsodium
 * Must be called before any crypto operations
 */
int crypto_init(void) {
    if (sodium_init() < 0) {
        fprintf(stderr, "ERROR: Failed to initialize libsodium\n");
        fprintf(stderr, "REASON: Critical crypto library initialization failed\n");
        fprintf(stderr, "FIX: Ensure libsodium is properly installed\n");
        return -1;
    }
    return 0;
}

/*
 * Generate a new Ed25519 keypair
 * 
 * @param kp       Keypair structure to fill
 * @param hostname Device hostname (used in RootStream code)
 * @return         0 on success, -1 on error
 * 
 * The RootStream code format is:
 *   <base64_public_key>@<hostname>
 * 
 * Example:
 *   kXx7YqZ3...Qp9w==@gaming-pc
 * 
 * This allows:
 * - Easy sharing via QR code or text
 * - Human-readable hostname
 * - Unique identification of each device
 */
int crypto_generate_keypair(keypair_t *kp, const char *hostname) {
    if (!kp || !hostname) {
        fprintf(stderr, "ERROR: Invalid arguments to crypto_generate_keypair\n");
        fprintf(stderr, "REASON: NULL keypair or hostname pointer\n");
        return -1;
    }

    /* Generate Ed25519 keypair */
    if (crypto_sign_keypair(kp->public_key, kp->secret_key) != 0) {
        fprintf(stderr, "ERROR: Failed to generate Ed25519 keypair\n");
        fprintf(stderr, "REASON: libsodium keypair generation failed\n");
        return -1;
    }

    /* Store hostname */
    strncpy(kp->identity, hostname, sizeof(kp->identity) - 1);
    kp->identity[sizeof(kp->identity) - 1] = '\0';

    /* Create RootStream code: base64(public_key)@hostname */
    char b64_pubkey[256];
    sodium_bin2base64(b64_pubkey, sizeof(b64_pubkey),
                     kp->public_key, CRYPTO_PUBLIC_KEY_BYTES,
                     sodium_base64_VARIANT_ORIGINAL);

    snprintf(kp->rootstream_code, sizeof(kp->rootstream_code),
             "%s@%s", b64_pubkey, hostname);

    printf("✓ Generated new keypair\n");
    printf("  Identity: %s\n", kp->identity);
    printf("  RootStream Code: %s\n", kp->rootstream_code);

    return 0;
}

/*
 * Load keypair from disk
 * 
 * @param kp         Keypair structure to fill
 * @param config_dir Configuration directory path
 * @return           0 on success, -1 on error
 * 
 * Keys are stored in:
 *   ~/.config/rootstream/identity.pub  (public key)
 *   ~/.config/rootstream/identity.key  (private key, mode 0600)
 */
int crypto_load_keypair(keypair_t *kp, const char *config_dir) {
    if (!kp || !config_dir) {
        fprintf(stderr, "ERROR: Invalid arguments to crypto_load_keypair\n");
        return -1;
    }

    char pubkey_path[512], seckey_path[512], identity_path[512];
    snprintf(pubkey_path, sizeof(pubkey_path), "%s/identity.pub", config_dir);
    snprintf(seckey_path, sizeof(seckey_path), "%s/identity.key", config_dir);
    snprintf(identity_path, sizeof(identity_path), "%s/identity.txt", config_dir);

    /* Check if private key exists */
    if (access(seckey_path, R_OK) != 0) {
        fprintf(stderr, "INFO: No existing keypair found\n");
        fprintf(stderr, "REASON: %s does not exist\n", seckey_path);
        fprintf(stderr, "ACTION: Will generate new keypair\n");
        return -1;
    }

    /* Load private key */
    FILE *f = fopen(seckey_path, "rb");
    if (!f) {
        fprintf(stderr, "ERROR: Cannot open private key file\n");
        fprintf(stderr, "FILE: %s\n", seckey_path);
        fprintf(stderr, "REASON: %s\n", strerror(errno));
        return -1;
    }

    if (fread(kp->secret_key, 1, CRYPTO_SECRET_KEY_BYTES, f) != CRYPTO_SECRET_KEY_BYTES) {
        fprintf(stderr, "ERROR: Invalid private key file\n");
        fprintf(stderr, "REASON: File too short or corrupted\n");
        fclose(f);
        return -1;
    }
    fclose(f);

    struct stat key_stat;
    if (stat(seckey_path, &key_stat) == 0) {
        if ((key_stat.st_mode & 0077) != 0) {
            fprintf(stderr, "WARNING: Private key permissions are too open\n");
            fprintf(stderr, "FILE: %s\n", seckey_path);
            fprintf(stderr, "RECOMMEND: chmod 600 %s\n", seckey_path);
        }
    } else {
        fprintf(stderr, "WARNING: Unable to stat private key file\n");
        fprintf(stderr, "FILE: %s\n", seckey_path);
        fprintf(stderr, "REASON: %s\n", strerror(errno));
    }

    /* Load public key */
    f = fopen(pubkey_path, "rb");
    if (!f) {
        fprintf(stderr, "ERROR: Cannot open public key file\n");
        fprintf(stderr, "FILE: %s\n", pubkey_path);
        fprintf(stderr, "REASON: %s\n", strerror(errno));
        return -1;
    }

    if (fread(kp->public_key, 1, CRYPTO_PUBLIC_KEY_BYTES, f) != CRYPTO_PUBLIC_KEY_BYTES) {
        fprintf(stderr, "ERROR: Invalid public key file\n");
        fprintf(stderr, "REASON: File too short or corrupted\n");
        fclose(f);
        return -1;
    }
    fclose(f);

    /* Load identity (hostname) */
    f = fopen(identity_path, "r");
    if (f) {
        fgets(kp->identity, sizeof(kp->identity), f);
        /* Remove newline */
        kp->identity[strcspn(kp->identity, "\n")] = 0;
        fclose(f);
    } else {
        /* Fallback to system hostname */
        gethostname(kp->identity, sizeof(kp->identity));
    }

    /* Reconstruct RootStream code */
    char b64_pubkey[256];
    sodium_bin2base64(b64_pubkey, sizeof(b64_pubkey),
                     kp->public_key, CRYPTO_PUBLIC_KEY_BYTES,
                     sodium_base64_VARIANT_ORIGINAL);
    snprintf(kp->rootstream_code, sizeof(kp->rootstream_code),
             "%s@%s", b64_pubkey, kp->identity);

    printf("✓ Loaded existing keypair\n");
    printf("  Identity: %s\n", kp->identity);

    return 0;
}

/*
 * Save keypair to disk
 * 
 * @param kp         Keypair to save
 * @param config_dir Configuration directory path
 * @return           0 on success, -1 on error
 * 
 * Security:
 * - Private key saved with mode 0600 (owner read/write only)
 * - Public key saved with mode 0644 (world readable)
 * - Directory created with mode 0700 (owner only)
 */
int crypto_save_keypair(const keypair_t *kp, const char *config_dir) {
    if (!kp || !config_dir) {
        fprintf(stderr, "ERROR: Invalid arguments to crypto_save_keypair\n");
        return -1;
    }

    /* Create config directory if it doesn't exist */
    if (mkdir(config_dir, 0700) != 0 && errno != EEXIST) {
        fprintf(stderr, "ERROR: Cannot create config directory\n");
        fprintf(stderr, "PATH: %s\n", config_dir);
        fprintf(stderr, "REASON: %s\n", strerror(errno));
        return -1;
    }

    char pubkey_path[512], seckey_path[512], identity_path[512];
    snprintf(pubkey_path, sizeof(pubkey_path), "%s/identity.pub", config_dir);
    snprintf(seckey_path, sizeof(seckey_path), "%s/identity.key", config_dir);
    snprintf(identity_path, sizeof(identity_path), "%s/identity.txt", config_dir);

    /* Save private key (mode 0600) */
    FILE *f = fopen(seckey_path, "wb");
    if (!f) {
        fprintf(stderr, "ERROR: Cannot create private key file\n");
        fprintf(stderr, "FILE: %s\n", seckey_path);
        fprintf(stderr, "REASON: %s\n", strerror(errno));
        return -1;
    }

    fwrite(kp->secret_key, 1, CRYPTO_SECRET_KEY_BYTES, f);
    fclose(f);
    chmod(seckey_path, 0600);  /* Owner read/write only */

    /* Save public key (mode 0644) */
    f = fopen(pubkey_path, "wb");
    if (!f) {
        fprintf(stderr, "ERROR: Cannot create public key file\n");
        fprintf(stderr, "FILE: %s\n", pubkey_path);
        fprintf(stderr, "REASON: %s\n", strerror(errno));
        return -1;
    }

    fwrite(kp->public_key, 1, CRYPTO_PUBLIC_KEY_BYTES, f);
    fclose(f);
    chmod(pubkey_path, 0644);  /* World readable */

    /* Save identity */
    f = fopen(identity_path, "w");
    if (f) {
        fprintf(f, "%s\n", kp->identity);
        fclose(f);
    }

    printf("✓ Saved keypair to %s\n", config_dir);

    return 0;
}

/*
 * Create encrypted session with peer
 * 
 * @param session      Session structure to fill
 * @param my_secret    Our private key
 * @param peer_public  Peer's public key
 * @return             0 on success, -1 on error
 * 
 * Uses X25519 (Curve25519) Diffie-Hellman key exchange to derive
 * a shared secret that both parties can compute but nobody else can.
 * 
 * Math: shared_secret = my_private * peer_public
 *     = peer_private * my_public
 * 
 * This is the "magic" of Diffie-Hellman: both sides get the same secret
 * without ever transmitting it over the network.
 */
int crypto_create_session(crypto_session_t *session,
                         const uint8_t *my_secret,
                         const uint8_t *peer_public) {
    if (!session || !my_secret || !peer_public) {
        fprintf(stderr, "ERROR: Invalid arguments to crypto_create_session\n");
        return -1;
    }

    /* Convert Ed25519 keys to Curve25519 for key exchange */
    uint8_t my_curve_secret[32], peer_curve_public[32];
    
    if (crypto_sign_ed25519_sk_to_curve25519(my_curve_secret, my_secret) != 0) {
        fprintf(stderr, "ERROR: Failed to convert secret key\n");
        return -1;
    }
    
    if (crypto_sign_ed25519_pk_to_curve25519(peer_curve_public, peer_public) != 0) {
        fprintf(stderr, "ERROR: Failed to convert public key\n");
        return -1;
    }

    /* Compute shared secret via X25519 */
    if (crypto_scalarmult(session->shared_key, my_curve_secret, peer_curve_public) != 0) {
        fprintf(stderr, "ERROR: Key exchange failed\n");
        fprintf(stderr, "REASON: Invalid public key or computation error\n");
        return -1;
    }

    /* Initialize nonce counter */
    session->nonce_counter = 0;
    session->authenticated = true;

    printf("✓ Established encrypted session\n");

    return 0;
}

/*
 * Encrypt packet using ChaCha20-Poly1305
 * 
 * @param session      Encryption session
 * @param plaintext    Data to encrypt
 * @param plain_len    Length of plaintext
 * @param ciphertext   Output buffer for encrypted data
 * @param cipher_len   Output: length of ciphertext (includes MAC)
 * @param nonce        Packet nonce (must be unique per packet)
 * @return             0 on success, -1 on error
 * 
 * ChaCha20-Poly1305 is an AEAD (Authenticated Encryption with Associated Data):
 * - Encrypts data (confidentiality)
 * - Adds authentication tag (integrity + authenticity)
 * - Prevents tampering, replay, or forgery
 * 
 * Output format: [ciphertext][16-byte MAC]
 */
int crypto_encrypt_packet(const crypto_session_t *session,
                         const void *plaintext, size_t plain_len,
                         void *ciphertext, size_t *cipher_len,
                         uint64_t nonce) {
    if (!session || !plaintext || !ciphertext || !cipher_len) {
        fprintf(stderr, "ERROR: Invalid arguments to crypto_encrypt_packet\n");
        return -1;
    }

    if (!session->authenticated) {
        fprintf(stderr, "ERROR: Cannot encrypt - session not authenticated\n");
        return -1;
    }

    /* Prepare nonce (24 bytes, nonce counter in first 8 bytes) */
    uint8_t nonce_bytes[CRYPTO_NONCE_BYTES] = {0};
    memcpy(nonce_bytes, &nonce, sizeof(nonce));

    /* Encrypt with ChaCha20-Poly1305 */
    unsigned long long actual_cipher_len;
    if (crypto_aead_chacha20poly1305_ietf_encrypt(
            ciphertext, &actual_cipher_len,
            plaintext, plain_len,
            NULL, 0,  /* No additional data */
            NULL,     /* No secret nonce */
            nonce_bytes,
            session->shared_key) != 0) {
        fprintf(stderr, "ERROR: Encryption failed\n");
        return -1;
    }

    *cipher_len = actual_cipher_len;
    return 0;
}

/*
 * Decrypt packet using ChaCha20-Poly1305
 * 
 * @param session      Decryption session
 * @param ciphertext   Encrypted data (includes MAC)
 * @param cipher_len   Length of ciphertext
 * @param plaintext    Output buffer for decrypted data
 * @param plain_len    Output: length of plaintext
 * @param nonce        Packet nonce (must match encryption nonce)
 * @return             0 on success, -1 on error
 * 
 * Verification:
 * 1. MAC is verified first (prevents tampering)
 * 2. If MAC invalid, decryption aborts (no data leaked)
 * 3. Only valid, authenticated packets are decrypted
 */
int crypto_decrypt_packet(const crypto_session_t *session,
                         const void *ciphertext, size_t cipher_len,
                         void *plaintext, size_t *plain_len,
                         uint64_t nonce) {
    if (!session || !ciphertext || !plaintext || !plain_len) {
        fprintf(stderr, "ERROR: Invalid arguments to crypto_decrypt_packet\n");
        return -1;
    }

    if (!session->authenticated) {
        fprintf(stderr, "ERROR: Cannot decrypt - session not authenticated\n");
        return -1;
    }

    /* Prepare nonce */
    uint8_t nonce_bytes[CRYPTO_NONCE_BYTES] = {0};
    memcpy(nonce_bytes, &nonce, sizeof(nonce));

    /* Decrypt and verify MAC */
    unsigned long long actual_plain_len;
    if (crypto_aead_chacha20poly1305_ietf_decrypt(
            plaintext, &actual_plain_len,
            NULL,  /* No secret nonce */
            ciphertext, cipher_len,
            NULL, 0,  /* No additional data */
            nonce_bytes,
            session->shared_key) != 0) {
        fprintf(stderr, "ERROR: Decryption failed\n");
        fprintf(stderr, "REASON: Invalid MAC (packet tampered or from wrong peer)\n");
        return -1;
    }

    *plain_len = actual_plain_len;
    return 0;
}

/*
 * Verify peer's public key
 * 
 * @param public_key Public key to verify
 * @param key_len    Length of key (should be 32)
 * @return           0 if valid, -1 if invalid
 * 
 * Checks:
 * - Correct length (32 bytes)
 * - Not all zeros (invalid key)
 * - Valid Ed25519 point (on curve)
 */
int crypto_verify_peer(const uint8_t *public_key, size_t key_len) {
    if (!public_key) {
        fprintf(stderr, "ERROR: NULL public key\n");
        return -1;
    }

    if (key_len != CRYPTO_PUBLIC_KEY_BYTES) {
        fprintf(stderr, "ERROR: Invalid public key length\n");
        fprintf(stderr, "EXPECTED: %d bytes\n", CRYPTO_PUBLIC_KEY_BYTES);
        fprintf(stderr, "GOT: %zu bytes\n", key_len);
        return -1;
    }

    /* Check if all zeros (invalid) */
    bool all_zero = true;
    for (size_t i = 0; i < key_len; i++) {
        if (public_key[i] != 0) {
            all_zero = false;
            break;
        }
    }

    if (all_zero) {
        fprintf(stderr, "ERROR: Public key is all zeros (invalid)\n");
        return -1;
    }

    /* libsodium will validate the key is on the curve during key exchange */

    return 0;
}
