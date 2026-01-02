#ifndef KEYS_H
#define KEYS_H

#include <stdint.h>
#include "crypto.h"

extern uint8_t K_DEVICE[16];
static const uint8_t K_MASTER[16] = {
    0x91,0xA3,0x44,0x10,0x77,0x5E,0xC9,0x02,
    0x6B,0x88,0xD2,0x19,0xF0,0xEE,0x31,0x4A
};

// DEVICE_ID artık const uint32_t
static const uint32_t DEVICE_ID = 0x23A9F1C4;

void derive_device_key(void);

#endif
