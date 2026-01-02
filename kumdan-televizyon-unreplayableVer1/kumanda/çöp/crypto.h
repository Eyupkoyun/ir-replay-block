#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <string.h>

/* ===== AES-128 ===== */
/* Compact public-domain AES (single-block, ECB) */

#define AES_BLOCK_SIZE 16

void aes128_encrypt_block(const uint8_t *key,
                          const uint8_t *input,
                          uint8_t *output);

/* ===== AES-CMAC ===== */

void aes_cmac(const uint8_t *key,
              const uint8_t *msg,
              uint32_t len,
              uint8_t *mac_out);

#endif
