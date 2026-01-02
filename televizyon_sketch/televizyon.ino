#define TSOP_PIN 2
#define IR_LED   9
#define LED_PIN  5

#define SECRET_KEY 0x5A

volatile unsigned long lastChange = 0;
volatile unsigned long pulse;
volatile bool pulseReady = false;
volatile bool levelNow;

uint8_t rxByte;
bool byteReady = false;

void carrier_on() {
    TCCR1A = _BV(COM1A0);
    TCCR1B = _BV(WGM12) | _BV(CS10);
    OCR1A  = 210;
}

void carrier_off() {
    TCCR1A = 0;
    TCCR1B = 0;
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
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(IR_LED, HIGH);
    attachInterrupt(digitalPinToInterrupt(TSOP_PIN), irISR, CHANGE);
    randomSeed(analogRead(0));
}

void loop() {
    receiveByte();
    if (!byteReady) return;
    byteReady = false;

    if (rxByte == 0xA0) {                 // REQUEST
        uint8_t challenge = random(1,255);
        sendByte(challenge);

        while (!byteReady)
            receiveByte();

        uint8_t response = rxByte;
        uint8_t expected = challenge ^ SECRET_KEY;

        if (response == expected)
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
}
