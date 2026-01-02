#include <Arduino.h>


#define IR_LED 3
#define BTN    7

uint8_t counter = 0x01;   // rolling code

void mark(unsigned int us) {
    unsigned long t = micros();
    while (micros() - t < us) {
        digitalWrite(IR_LED, LOW);
        delayMicroseconds(13);
        digitalWrite(IR_LED, HIGH);
        delayMicroseconds(13);
    }
}

void space(unsigned int us) {
    digitalWrite(IR_LED, HIGH);
    delayMicroseconds(us);
}

void sendByte(uint8_t d) {
    for (int i = 7; i >= 0; i--) {
        if (d & (1 << i))
            mark(1680);   // 1
        else
            mark(560);    // 0
        space(560);
    }
}

void sendFrame(uint8_t value) {
    mark(9000);
    space(4500);
    sendByte(value);
    mark(560);
    digitalWrite(IR_LED, HIGH);
}

void setup() {
    pinMode(IR_LED, OUTPUT);
    pinMode(BTN, INPUT_PULLUP);
    digitalWrite(IR_LED, HIGH);
}

void loop() {
    if (digitalRead(BTN) == LOW) {
        sendFrame(counter);
        counter++;                // kritik nokta
        delay(300);
    }
}



/*#include <Arduino.h>

#define IR_LED_PIN    7
#define TSOP_PIN      2
#define BUTTON_PIN    5

bool armed = false;
uint64_t pending_nonce = 0;

// ---- LOW LEVEL IR TX ---- //
void ir_mark(uint16_t us) {
    uint32_t start = micros();
    while (micros() - start < us) {
        digitalWrite(IR_LED_PIN, HIGH);
        delayMicroseconds(13);
        digitalWrite(IR_LED_PIN, LOW);
        delayMicroseconds(13);
    }
}

void ir_space(uint16_t us) {
    digitalWrite(IR_LED_PIN, LOW);
    delayMicroseconds(us);
}

void send_response(uint8_t *buf, int len) {
    // RX geçici durdurulabilir (mevcut sistemde basit digitalRead kullanıyoruz)
    // Paket gönderimi
    ir_mark(9000);
    ir_space(4500);

    for (int i = 0; i < len; i++) {
        for (int b = 0; b < 8; b++) {
            ir_mark(560);
            ir_space((buf[i] & (1 << b)) ? 1690 : 560);
        }
    }

    ir_mark(560);
    ir_space(0);
}

// ---- RX SIMPLIFIED ---- //
// TSOP’dan nonce okuma (sadece örnek, kendi protokole göre değiştir)
bool read_tsop(uint64_t *nonce) {
    // TSOP LOW/ HIGH sürelerini ölç ve basit threshold ile 1/0 belirle
    static uint8_t bit = 0;
    static uint64_t n = 0;

    if (digitalRead(TSOP_PIN) == LOW) {
        n |= ((uint64_t)1 << bit);
    }

    bit++;
    if (bit >= 64) {
        *nonce = n;
        bit = 0;
        n = 0;
        return true;
    }
    return false;
}

void setup() {
    pinMode(IR_LED_PIN, OUTPUT);
    pinMode(TSOP_PIN, INPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
    // Buton tetikleme
    static uint8_t last_btn = HIGH;
    uint8_t btn = digitalRead(BUTTON_PIN);
    if (last_btn == HIGH && btn == LOW) {
        armed = true;
    }
    last_btn = btn;

    // TSOP okuma
    uint64_t nonce;
    if (read_tsop(&nonce)) {
        pending_nonce = nonce;
    }

    // Gönderim
    if (armed && pending_nonce != 0) {
        armed = false;

        uint8_t pkt[29] = {0};
        pkt[0] = 0x01;
        // Örnek veri doldurma: nonce kopyası (senin protokole göre değiştir)
        memcpy(&pkt[1], &pending_nonce, 8);

        send_response(pkt, 29);
        pending_nonce = 0;
    }
}

*/




/*
#include <Arduino.h>
#include <IRremote.h>
#include "crypto.h"
#include "keys.h"

// ===== PIN ===== //
#define REM_IR_LED_PIN   3
#define REM_TSOP_PIN     4
#define REM_BUTTON_PIN   7

//===== STATE ===== //
static bool armed = false;
static bool have_nonce = false;
static bool rx_enabled = true;

static uint64_t pending_nonce = 0;
static uint32_t remote_ctr = 0;

// ===== RX CONTROL ===== //
static inline void rx_off() {
    IrReceiver.stop();
    rx_enabled = false;
}

static inline void rx_on() {
    IrReceiver.start();
    rx_enabled = true;
}

// ===== LOW LEVEL TX (38 kHz) ===== //
static inline void ir_mark(uint16_t us) {
    uint32_t t = micros();
    while (micros() - t < us) {
        digitalWrite(REM_IR_LED_PIN, HIGH);
        delayMicroseconds(13);
        digitalWrite(REM_IR_LED_PIN, LOW);
        delayMicroseconds(13);
    }
}

static inline void ir_space(uint16_t us) {
    digitalWrite(REM_IR_LED_PIN, LOW);
    delayMicroseconds(us);
}

// ===== RX : CHALLENGE =====//
bool ir_receive_challenge(uint64_t *nonce) {

    if (!rx_enabled)
        return false;

    if (!IrReceiver.decode())
        return false;

    int rawlen = IrReceiver.irparams.rawlen;
    if (rawlen < 70) {
        IrReceiver.resume();
        return false;
    }

    IRRawbufType *raw = IrReceiver.irparams.rawbuf;

    uint64_t n = 0;
    int bit = 0;

    for (int i = 1; i < rawlen && bit < 64; i += 2) {

        uint16_t duration_us = raw[i] * MICROS_PER_TICK;

        if (duration_us > 1000)
            n |= ((uint64_t)1 << bit);

        bit++;
    }

    *nonce = n;
    IrReceiver.resume();
    return true;
}



// ===== TX : RESPONSE ===== //
void ir_send_response(uint8_t *buf) {

    rx_off();                  // RX KESİN KAPALI

    ir_mark(9000);
    ir_space(4500);

    for (int i = 0; i < 29; i++) {
        for (int b = 0; b < 8; b++) {
            ir_mark(560);
            ir_space((buf[i] & (1 << b)) ? 1690 : 560);
        }
    }

    ir_mark(560);
    ir_space(0);

    delay(5);                  // TSOP AGC settle
    rx_on();                   // RX GERİ AÇILIR
}

//===== SETUP ===== //
void setup() {

    pinMode(REM_IR_LED_PIN, OUTPUT);
    pinMode(REM_TSOP_PIN, INPUT);
    pinMode(REM_BUTTON_PIN, INPUT_PULLUP);

    IrReceiver.begin(REM_TSOP_PIN, ENABLE_LED_FEEDBACK);
    rx_on();

    derive_device_key();
}

// ===== LOOP ===== //
void loop() {

    // ---- BUTTON ----//
    static uint8_t last_btn = HIGH;
    uint8_t btn = digitalRead(REM_BUTTON_PIN);

    if (last_btn == HIGH && btn == LOW)
        armed = true;

    last_btn = btn;

    /// ---- RECEIVE CHALLENGE ---- //
    uint64_t nonce;
    if (ir_receive_challenge(&nonce)) {
        pending_nonce = nonce;
        have_nonce = true;
    }

    // ---- SEND RESPONSE ---- //
    if (!armed || !have_nonce)
        return;

    armed = false;
    have_nonce = false;
    remote_ctr++;

    uint8_t pkt[29] = {0};
    pkt[0] = 0x01;
    memcpy(&pkt[1], &remote_ctr, 4);
    memcpy(&pkt[5], &pending_nonce, 8);
    aes_cmac(K_DEVICE, pkt, 13, &pkt[13]);

    ir_send_response(pkt);
}
*/