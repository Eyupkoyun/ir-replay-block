#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "crypto.h"
#include "keys.h"

uint8_t K_DEVICE[16];

static uint64_t tv_nonce;
static uint32_t last_remote_ctr;
static bool challenge_valid;

/* ---- DONANIM STUB ---- */
uint64_t rng64(void){ return 0xA5A5A5A512345678ULL; }
void ir_send_challenge(uint64_t nonce){}
bool ir_receive_response(uint8_t *buf){ return false; }
void led_on(void){}

/* ---- TV ---- */

void tv_send_challenge(void)
{
    tv_nonce=rng64();
    challenge_valid=true;
    ir_send_challenge(tv_nonce);
}

void tv_process(void)
{
    uint8_t rx[32];
    if(!ir_receive_response(rx)) return;

    uint8_t cmd=rx[0];
    uint32_t ctr;
    uint64_t nonce;
    memcpy(&ctr,&rx[1],4);
    memcpy(&nonce,&rx[5],8);
    uint8_t *mac_rx=&rx[13];

    if(!challenge_valid) return;
    if(nonce!=tv_nonce) return;
    if(ctr<=last_remote_ctr) return;

    uint8_t msg[13];
    memcpy(msg,&rx[0],13);

    uint8_t mac_calc[16];
    aes_cmac(K_DEVICE,msg,13,mac_calc);

    if(memcmp(mac_rx,mac_calc,16)==0){
        last_remote_ctr=ctr;
        challenge_valid=false;
        led_on();
    }
}

int main(void)
{
    derive_device_key();
    while(1){
        tv_send_challenge();
        tv_process();
    }
}
