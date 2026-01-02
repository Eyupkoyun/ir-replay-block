#define IR_PIN 2
#define LED_PIN 5

#define START_MARK_MIN 8000
#define START_SPACE_MIN 4000
#define BIT_THRESHOLD   1300

volatile unsigned long lastChange = 0;
volatile unsigned long pulse = 0;
volatile bool pulseReady = false;
volatile bool levelNow = HIGH;

enum {
    IDLE,
    WAIT_START_SPACE,
    RECEIVE_BITS
} state = IDLE;

uint8_t data = 0;
int bitPos = 7;

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

    // LOW finished
    if (levelNow == HIGH) {
        if (state == IDLE && pulse > START_MARK_MIN) {
            state = WAIT_START_SPACE;
            return;
        }

        if (state == RECEIVE_BITS) {
            if (pulse > BIT_THRESHOLD)
                data |= (1 << bitPos);

            bitPos--;

            if (bitPos < 0) {
                if (data == 0xA5)
                    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

                state = IDLE;
            }
        }
    }
    // HIGH finished
    else {
        if (state == WAIT_START_SPACE && pulse > START_SPACE_MIN) {
            state = RECEIVE_BITS;
            data = 0;
            bitPos = 7;
        }
    }
}
