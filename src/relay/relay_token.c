/*
 * relay_token.c — Portable HMAC-SHA256 relay auth token implementation
 *
 * SHA-256 core derived from the public-domain implementation by
 * Brad Conte (https://github.com/B-Con/crypto-algorithms).
 */

#include "relay_token.h"

#include <string.h>
#include <stdlib.h>

/* ── SHA-256 ──────────────────────────────────────────────────────── */

typedef struct {
    uint8_t  data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
} sha256_ctx_t;

#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))
#define CH(x,y,z)     (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z)    (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)        (ROTRIGHT(x,2)  ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x)        (ROTRIGHT(x,6)  ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x)       (ROTRIGHT(x,7)  ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x)       (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,
    0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
    0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,
    0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,
    0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
    0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,
    0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,
    0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
    0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2,
};

static void sha256_transform(sha256_ctx_t *ctx, const uint8_t *data) {
    uint32_t a,b,c,d,e,f,g,h,i,j,t1,t2,m[64];

    for (i=0,j=0; i<16; ++i,j+=4)
        m[i] = ((uint32_t)data[j]<<24)|((uint32_t)data[j+1]<<16)
              |((uint32_t)data[j+2]<<8)|(uint32_t)data[j+3];
    for (; i<64; ++i)
        m[i] = SIG1(m[i-2]) + m[i-7] + SIG0(m[i-15]) + m[i-16];

    a=ctx->state[0]; b=ctx->state[1]; c=ctx->state[2]; d=ctx->state[3];
    e=ctx->state[4]; f=ctx->state[5]; g=ctx->state[6]; h=ctx->state[7];

    for (i=0; i<64; ++i) {
        t1 = h + EP1(e) + CH(e,f,g) + K[i] + m[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }

    ctx->state[0]+=a; ctx->state[1]+=b; ctx->state[2]+=c; ctx->state[3]+=d;
    ctx->state[4]+=e; ctx->state[5]+=f; ctx->state[6]+=g; ctx->state[7]+=h;
}

static void sha256_init(sha256_ctx_t *ctx) {
    ctx->datalen=0; ctx->bitlen=0;
    ctx->state[0]=0x6a09e667; ctx->state[1]=0xbb67ae85;
    ctx->state[2]=0x3c6ef372; ctx->state[3]=0xa54ff53a;
    ctx->state[4]=0x510e527f; ctx->state[5]=0x9b05688c;
    ctx->state[6]=0x1f83d9ab; ctx->state[7]=0x5be0cd19;
}

static void sha256_update(sha256_ctx_t *ctx, const uint8_t *data, size_t len) {
    for (size_t i=0; i<len; ++i) {
        ctx->data[ctx->datalen++] = data[i];
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(sha256_ctx_t *ctx, uint8_t *hash) {
    uint32_t i = ctx->datalen;
    ctx->data[i++] = 0x80;
    if (ctx->datalen < 56) {
        while (i<56) ctx->data[i++]=0;
    } else {
        while (i<64) ctx->data[i++]=0;
        sha256_transform(ctx,ctx->data);
        memset(ctx->data,0,56);
    }
    ctx->bitlen += ctx->datalen * 8;
    ctx->data[63]=(uint8_t)(ctx->bitlen);
    ctx->data[62]=(uint8_t)(ctx->bitlen>>8);
    ctx->data[61]=(uint8_t)(ctx->bitlen>>16);
    ctx->data[60]=(uint8_t)(ctx->bitlen>>24);
    ctx->data[59]=(uint8_t)(ctx->bitlen>>32);
    ctx->data[58]=(uint8_t)(ctx->bitlen>>40);
    ctx->data[57]=(uint8_t)(ctx->bitlen>>48);
    ctx->data[56]=(uint8_t)(ctx->bitlen>>56);
    sha256_transform(ctx,ctx->data);
    for (i=0; i<4; ++i) {
        hash[i]    = (ctx->state[0]>>(24-i*8)) & 0xFF;
        hash[i+4]  = (ctx->state[1]>>(24-i*8)) & 0xFF;
        hash[i+8]  = (ctx->state[2]>>(24-i*8)) & 0xFF;
        hash[i+12] = (ctx->state[3]>>(24-i*8)) & 0xFF;
        hash[i+16] = (ctx->state[4]>>(24-i*8)) & 0xFF;
        hash[i+20] = (ctx->state[5]>>(24-i*8)) & 0xFF;
        hash[i+24] = (ctx->state[6]>>(24-i*8)) & 0xFF;
        hash[i+28] = (ctx->state[7]>>(24-i*8)) & 0xFF;
    }
}

static void sha256(const uint8_t *data, size_t len, uint8_t *out) {
    sha256_ctx_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, out);
}

/* ── HMAC-SHA256 ─────────────────────────────────────────────────── */

static void hmac_sha256(const uint8_t *key,   size_t key_len,
                         const uint8_t *msg,   size_t msg_len,
                         uint8_t       *out) {
    uint8_t k[64] = {0};
    uint8_t ipad[64], opad[64];
    uint8_t inner_hash[32];

    if (key_len > 64) {
        sha256(key, key_len, k);
    } else {
        memcpy(k, key, key_len);
    }

    for (int i=0; i<64; i++) { ipad[i] = k[i] ^ 0x36; opad[i] = k[i] ^ 0x5c; }

    /* inner = SHA256(ipad || msg) */
    sha256_ctx_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, ipad, 64);
    sha256_update(&ctx, msg, msg_len);
    sha256_final(&ctx, inner_hash);

    /* outer = SHA256(opad || inner_hash) */
    sha256_init(&ctx);
    sha256_update(&ctx, opad, 64);
    sha256_update(&ctx, inner_hash, 32);
    sha256_final(&ctx, out);
}

/* ── Public API ───────────────────────────────────────────────────── */

void relay_token_generate(const uint8_t *key,
                           const uint8_t *peer_pubkey,
                           const uint8_t *nonce,
                           uint8_t       *out_token) {
    if (!key || !peer_pubkey || !nonce || !out_token) return;

    /* Message = peer_pubkey (32) || nonce (8) */
    uint8_t msg[40];
    memcpy(msg,      peer_pubkey, 32);
    memcpy(msg + 32, nonce,       8);

    hmac_sha256(key, RELAY_KEY_BYTES, msg, sizeof(msg), out_token);
}

bool relay_token_validate(const uint8_t *expected,
                           const uint8_t *provided) {
    if (!expected || !provided) return false;

    /* Constant-time comparison */
    uint8_t diff = 0;
    for (int i = 0; i < RELAY_TOKEN_BYTES; i++) {
        diff |= expected[i] ^ provided[i];
    }
    return diff == 0;
}
