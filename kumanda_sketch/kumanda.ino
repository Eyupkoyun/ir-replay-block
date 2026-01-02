#define TSOP_PIN 4
#define IR_LED   3
#define BTN_PIN  7

#define SECRET_KEY 0x5A

volatile unsigned long lastChange = 0;
volatile unsigned long pulse;
volatile bool pulseReady = false;
volatile bool levelNow;

uint8_t rxByte;
bool byteReady = false;

void carrier_on() {
    TCCR2A = _BV(COM2B1) | _BV(WGM21);
    TCCR2B = _BV(CS20);
    OCR2A  = 210;
}

void carrier_off() {
    TCCR2A = 0;
    TCCR2B = 0;
    digitalWrite(IR_LED, HIGH);
}

void mark(unsigned int us) {
    carrier_on();
    delayMicroseconds(us);
}

void space(unsigned int us) {
    carrier_off();
    delayMicroseconds(us);
}

void sendByte(uint8_t d) {
    mark(9000); space(4500);
    for (int i = 7; i >= 0; i--) {
        if (d & (1 << i)) mark(1680);
        else              mark(560);
        space(560);
    }
    mark(560);
    carrier_off();
}

void irISR() {
    unsigned long now = micros();
    levelNow = digitalRead(TSOP_PIN);
    pulse = now - lastChange;
    lastChange = now;
    pulseReady = true;
}

void receiveByte() {
    static int bitPos = 7;
    static uint8_t data = 0;

    if (!pulseReady) return;
    pulseReady = false;

    if (levelNow == HIGH) {
        if (pulse > 8000) {
            bitPos = 7;
            data = 0;
            return;
        }
        if (pulse > 1000)
            data |= (1 << bitPos);
        bitPos--;
        if (bitPos < 0) {
            rxByte = data;
            byteReady = true;
        }
    }
}

void setup() {
    pinMode(TSOP_PIN, INPUT);
    pinMode(IR_LED, OUTPUT);
    pinMode(BTN_PIN, INPUT_PULLUP);
    digitalWrite(IR_LED, HIGH);
    attachInterrupt(digitalPinToInterrupt(TSOP_PIN), irISR, CHANGE);
}

void loop() {
    if (digitalRead(BTN_PIN) == LOW) {
        sendByte(0xA0);                  // REQUEST

        while (!byteReady)
            receiveByte();

        uint8_t challenge = rxByte;
        byteReady = false;

        uint8_t response = challenge ^ SECRET_KEY;
        sendByte(response);

        delay(500);
    }
}
