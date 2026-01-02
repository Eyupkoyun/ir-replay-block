#include <stdint.h>
#include <string.h>
#include "crypto.h"
#include "keys.h"

uint8_t K_DEVICE[16];
static uint32_t remote_ctr;

/* ---- DONANIM STUB ---- */
bool ir_receive_challenge(uint64_t *nonce){ return false; }
void ir_send_response(uint8_t *buf){}

/* ---- REMOTE ---- */

void remote_loop(void)
{
    uint64_t nonce;
    if(!ir_receive_challenge(&nonce)) return;

    remote_ctr++;

    uint8_t pkt[29];
    pkt[0]=0x01;               // CMD
    memcpy(&pkt[1],&remote_ctr,4);
    memcpy(&pkt[5],&nonce,8);

    aes_cmac(K_DEVICE,pkt,13,&pkt[13]);
    ir_send_response(pkt);
}

int main(void)
{
    derive_device_key();
    while(1) remote_loop();
}
