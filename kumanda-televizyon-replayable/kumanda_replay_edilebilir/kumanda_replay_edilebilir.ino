#define IR_LED 3
#define SENT_LED 2
#define BTN    7

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
        if (d & (1 << i)) {
            mark(1680);   // 1
        } else {
            mark(560);    // 0
        }
        space(560);
    }
}

void sendFrame() {
    mark(9000);
    space(4500);
    sendByte(0xA5);
    mark(560);
    digitalWrite(IR_LED, HIGH);
}

void setup() {
    pinMode(IR_LED, OUTPUT);
    pinMode(BTN, INPUT_PULLUP);
    pinMode(SENT_LED, OUTPUT);
    digitalWrite(IR_LED, HIGH);
}

void loop() {
    if (digitalRead(BTN) == LOW) {
        sendFrame();
        digitalWrite(SENT_LED, HIGH);
        delay(300);
        digitalWrite(SENT_LED, LOW);
    }
}
