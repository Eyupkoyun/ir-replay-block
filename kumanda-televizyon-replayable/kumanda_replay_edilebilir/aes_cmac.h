#pragma once
#include <stdint.h>

void aes_cmac(
    const uint8_t *key,
    const uint8_t *msg,
    uint16_t len,
    uint8_t *mac_out
);
