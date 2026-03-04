/*
 * config_serialiser.h — Binary stream config serialiser / deserialiser
 *
 * Extends stream_config_encode/decode with a versioned envelope:
 *
 *   Offset  Size  Field
 *    0       4    Envelope magic  0x53455256 ('SERV')
 *    4       2    Version         (major << 8 | minor)
 *    6       2    Payload length  (number of bytes that follow)
 *    8       N    Payload         (stream_config binary, N = SCFG_HDR_SIZE)
 *
 * The version field allows forward-compatibility: a decoder that sees
 * a newer major version returns CONFIG_ERR_VERSION.
 *
 * Thread-safety: stateless, thread-safe.
 */

#ifndef ROOTSTREAM_CONFIG_SERIALISER_H
#define ROOTSTREAM_CONFIG_SERIALISER_H

#include "stream_config.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CSER_ENVELOPE_MAGIC    0x53455256UL  /* 'SERV' */
#define CSER_ENVELOPE_HDR_SIZE 8
#define CSER_VERSION_MAJOR     1
#define CSER_VERSION_MINOR     0
#define CSER_VERSION           ((CSER_VERSION_MAJOR << 8) | CSER_VERSION_MINOR)

/** Error codes */
#define CSER_OK               0
#define CSER_ERR_NULL        -1
#define CSER_ERR_BUF_SMALL   -2
#define CSER_ERR_BAD_MAGIC   -3
#define CSER_ERR_VERSION     -4
#define CSER_ERR_PAYLOAD     -5

/**
 * config_serialiser_encode — wrap @cfg in a versioned envelope into @buf
 *
 * @param cfg     Config to encode
 * @param buf     Output buffer (>= CSER_ENVELOPE_HDR_SIZE + SCFG_HDR_SIZE)
 * @param buf_sz  Buffer size
 * @return        Bytes written, or CSER_ERR_* (negative) on error
 */
int config_serialiser_encode(const stream_config_t *cfg,
                               uint8_t               *buf,
                               size_t                 buf_sz);

/**
 * config_serialiser_decode — unwrap envelope and decode @cfg from @buf
 *
 * Validates envelope magic and major version before decoding.
 *
 * @param buf     Input buffer
 * @param buf_sz  Valid bytes in @buf
 * @param cfg     Output config
 * @return        CSER_OK on success, CSER_ERR_* on error
 */
int config_serialiser_decode(const uint8_t  *buf,
                               size_t          buf_sz,
                               stream_config_t *cfg);

/**
 * config_serialiser_total_size — total encoded size in bytes
 *
 * @return  CSER_ENVELOPE_HDR_SIZE + SCFG_HDR_SIZE
 */
static inline int config_serialiser_total_size(void) {
    return CSER_ENVELOPE_HDR_SIZE + SCFG_HDR_SIZE;
}

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CONFIG_SERIALISER_H */
