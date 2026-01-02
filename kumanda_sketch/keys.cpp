#include "keys.h"

uint8_t K_DEVICE[16];

void derive_device_key(void) {
    // &DEVICE_ID geçerli çünkü artık değişken
    aes128_encrypt_block(K_MASTER, (uint8_t*)&DEVICE_ID, K_DEVICE);
}
