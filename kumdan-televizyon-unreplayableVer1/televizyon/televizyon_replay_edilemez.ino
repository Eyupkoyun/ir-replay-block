#include <Arduino.h>


#define IR_PIN 2
#define LED_PIN 5

#define START_MARK_MIN 8000
#define START_SPACE_MIN 4000
#define BIT_THRESHOLD  1300

volatile unsigned long lastChange = 0;
volatile unsigned long pulse = 0;
volatile bool pulseReady = false;
volatile bool levelNow = HIGH;

enum { IDLE, WAIT_START_SPACE, RECEIVE_BITS } state = IDLE;

uint8_t data = 0;
int bitPos = 7;

uint8_t lastAccepted = 0x00;   // replay bariyeri

void irISR() {
    unsigned long now = micros();
    bool level = digitalRead(IR_PIN);
    pulse = now - lastChange;
    levelNow = level;
    pulseReady = true;
    lastChange = now;
}

void setup() {
    pinMode(IR_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(IR_PIN), irISR, CHANGE);
}

void loop() {
    if (!pulseReady) return;
    pulseReady = false;

    if (levelNow == HIGH) { // LOW finished
        if (state == IDLE && pulse > START_MARK_MIN) {
            state = WAIT_START_SPACE;
            return;
        }

        if (state == RECEIVE_BITS) {
            if (pulse > BIT_THRESHOLD)
                data |= (1 << bitPos);

            bitPos--;

            if (bitPos < 0) {
                if (data > lastAccepted) {   // replay check
                    lastAccepted = data;
                    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
                }
                state = IDLE;
            }
        }
    } 
    else { // HIGH finished
        if (state == WAIT_START_SPACE && pulse > START_SPACE_MIN) {
            state = RECEIVE_BITS;
            data = 0;
            bitPos = 7;
        }
    }
}



/*
#include <Arduino.h>
#include <IRremote.h>

//-------- PIN MAP --------//
#define PIN_TSOP_POWER 3
#define PIN_TSOP_OUT   2
#define PIN_IR_TX      7
#define PIN_LED_OK     5

//-------- CRYPTO --------//
static const uint32_t SECRET_KEY = 0xA5C3F1D7;

//-------- IR --------//
IRsend irsend;
IRrecv irrecv(PIN_TSOP_OUT);
decode_results results;

//------- UTILS --------//
uint32_t generate_nonce() {
    return micros() ^ 0x5A5A1234;
}

uint32_t compute_response(uint32_t nonce) {
    return nonce ^ SECRET_KEY;
}

//-------- TSOP CONTROL -------//
inline void tsop_enable() {
    digitalWrite(PIN_TSOP_POWER, HIGH);
}

inline void tsop_disable() {
    digitalWrite(PIN_TSOP_POWER, LOW);
}

//-------- SETUP -------- ////
void setup() {
    pinMode(PIN_TSOP_POWER, OUTPUT);
    pinMode(PIN_LED_OK, OUTPUT);

    digitalWrite(PIN_LED_OK, LOW);
    tsop_enable();

    irsend.begin(PIN_IR_TX);
    irrecv.enableIRIn();
}

// -------- LOOP --------//
void loop() {
    uint32_t nonce = generate_nonce();

    ---- SEND CHALLENGE ----
    tsop_disable();              // RX OFF
    delayMicroseconds(200);

    irsend.sendNEC(nonce, 32);

    delay(10);
    tsop_enable();               // RX ON

   // ---- WAIT RESPONSE ----//
    unsigned long t0 = millis();
    bool valid = false;

    while (millis() - t0 < 200) {
        if (irrecv.decode(&results)) {
            uint32_t received = results.value;
            uint32_t expected = compute_response(nonce);

            if (received == expected) {
                valid = true;
            }

            irrecv.resume();
            break;
        }
    }

    // ---- RESULT ----/
    if (valid) {
        digitalWrite(PIN_LED_OK, HIGH);
        delay(500);
        digitalWrite(PIN_LED_OK, LOW);
    }

    delay(1000);
}


/* 
#include <Arduino.h>
#include <IRremote.h>
#include "crypto.h"
#include "keys.h"

//===== PINLER ===== //
#define TV_TSOP_PIN   2     // TSOP OUT
#define TV_TSOP_PWR   3     // TSOP VCC (GPIO power)
#define TV_LED        5     // Normal LED
#define TV_IR_LED     7     // IR LED

// ===== STATE =====//
static uint32_t last_ctr = 0;

// ===== IR =====//
IRsend irsend(TV_IR_LED);
IRrecv irrecv(TV_TSOP_PIN);
decode_results results;

// ===== RNG ===== //
uint64_t rng64(void) {
    return ((uint64_t)random() << 32) | random();
}

//===== CHALLENGE TX ===== //
void ir_send_challenge(uint64_t nonce) {
    uint16_t raw[70];
    int i = 0;

    raw[i++] = 9000;
    raw[i++] = 4500;

    for (int b = 0; b < 64; b++) {
        raw[i++] = 560;
        raw[i++] = (nonce & ((uint64_t)1 << b)) ? 1690 : 560;
    }

    raw[i++] = 560;
    irsend.sendRaw(raw, i, 38);
}

//===== RESPONSE RX ===== //
bool ir_receive_response(uint8_t *pkt) {

    if (!irrecv.decode(&results))
        return false;

    if (results.rawlen < 200) {
        irrecv.resume();
        return false;
    }

    int bit = 0;
    for (int i = 1; i < results.rawlen && bit < 29 * 8; i += 2) {
        if (results.rawbuf[i] > 1000)
            pkt[bit / 8] |= (1 << (bit % 8));
        bit++;
    }

    irrecv.resume();
    return true;
}

//===== SETUP ===== 
void setup() {
    pinMode(TV_LED, OUTPUT);
    pinMode(TV_IR_LED, OUTPUT);
    pinMode(TV_TSOP_PIN, INPUT);
    pinMode(TV_TSOP_PWR, OUTPUT);

    digitalWrite(TV_TSOP_PWR, LOW);   // TSOP KAPALI

    irsend.begin(TV_IR_LED);
    irrecv.enableIRIn();

    derive_device_key();
    randomSeed(analogRead(A0));
}

//===== LOOP ===== 
void loop() {

    // ---- FAZ 1: TX ---- //
    digitalWrite(TV_TSOP_PWR, LOW);   // TSOP OFF
    delay(5);

    uint64_t nonce = rng64();
    ir_send_challenge(nonce);

    delay(30);                        // TX bitmesini bekle

     //---- FAZ 2: RX ----//
    digitalWrite(TV_TSOP_PWR, HIGH);  // TSOP ON
    delay(5);

    uint32_t t0 = millis();
    uint8_t rx[29] = {0};

    while (millis() - t0 < 150) {

        if (!ir_receive_response(rx))
            continue;

        uint32_t ctr;
        uint64_t rnonce;

        memcpy(&ctr, &rx[1], 4);
        memcpy(&rnonce, &rx[5], 8);

        if (rnonce != nonce) return;
        if (ctr <= last_ctr) return;

        uint8_t mac[16];
        aes_cmac(K_DEVICE, rx, 13, mac);

        if (memcmp(mac, &rx[13], 16) != 0)
            return;

        last_ctr = ctr;
        digitalWrite(TV_LED, HIGH);   // BAŞARILI
        return;
    }
}
*/