#include <Arduino.h>
#include "driver/rmt.h"

/* ================= PINLER ================= */
static const gpio_num_t RX_PIN   = GPIO_NUM_27;
static const gpio_num_t TX_PIN   = GPIO_NUM_26;
static const gpio_num_t TSOP_PWR = GPIO_NUM_33;

#define BTN_REPLAY 14
#define BTN_CLEAR  25
#define Led_sent  32

#define RX_CH RMT_CHANNEL_0
#define TX_CH RMT_CHANNEL_1

#define MAX_ITEMS 128

/* ================= STATE ================= */
volatile bool frame_ready = false;

/* ================= BUFFER ================= */
rmt_item32_t replay_items[MAX_ITEMS];
size_t replay_count = 0;
TaskHandle_t rxTaskHandle = NULL;

/* ================= RX INIT ================= */
void ir_rx_init() {
    rmt_config_t rx = {};
    rx.channel = RX_CH;
    rx.gpio_num = RX_PIN;
    rx.clk_div = 80;                // 1 µs
    rx.mem_block_num = 4;
    rx.rmt_mode = RMT_MODE_RX;

    rx.rx_config.idle_threshold = 12000; // 12 ms
    rx.rx_config.filter_en = true;
    rx.rx_config.filter_ticks_thresh = 50;

    rx.flags = RMT_CHANNEL_FLAGS_INVERT_SIG;

    rmt_config(&rx);
    rmt_driver_install(rx.channel, 4096, 0);
}

/* ================= TX INIT ================= */
void ir_tx_init() {
    rmt_config_t tx = {};
    tx.channel = TX_CH;
    tx.gpio_num = TX_PIN;
    tx.clk_div = 80;
    tx.mem_block_num = 2;
    tx.rmt_mode = RMT_MODE_TX;

    tx.tx_config.loop_en = false;
    tx.tx_config.carrier_en = true;
    tx.tx_config.carrier_freq_hz = 38000;
    tx.tx_config.carrier_duty_percent = 33;
    tx.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
    tx.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    tx.tx_config.idle_output_en = true;

    rmt_config(&tx);
    rmt_driver_install(tx.channel, 0, 0);
}

/* ================= RX TASK ================= */
void rx_task(void *arg) {
    RingbufHandle_t rb;
    rmt_get_ringbuf_handle(RX_CH, &rb);
    rmt_rx_start(RX_CH, true);

    while (true) {
        size_t size = 0;
        rmt_item32_t *items =
            (rmt_item32_t *)xRingbufferReceive(rb, &size, portMAX_DELAY);

        if (!items) continue;

        size_t count = size / sizeof(rmt_item32_t);
        if (count > MAX_ITEMS) count = MAX_ITEMS;

        if (!frame_ready && count > 6) {
            memcpy(replay_items, items, count * sizeof(rmt_item32_t));
            replay_count = count;
            frame_ready = true;

            Serial.print("FRAME CAPTURED: ");
            Serial.println(replay_count);
        }

        vRingbufferReturnItem(rb, items);
    }
}

/* ================= SETUP ================= */
void setup() {
    Serial.begin(115200);

    pinMode(BTN_REPLAY, INPUT_PULLUP);
    pinMode(BTN_CLEAR, INPUT_PULLUP);
    pinMode(Led_sent, OUTPUT);

    pinMode((int)TSOP_PWR, OUTPUT);
    digitalWrite((int)TSOP_PWR, HIGH);

    ir_rx_init();
    ir_tx_init();

    xTaskCreate(rx_task, "rx_task", 4096, NULL, 10, &rxTaskHandle);


    Serial.println("ESP32 IR REPLAY READY");
}

/* ================= LOOP ================= */
void loop() {

    /* CLEAR */
   if (digitalRead(BTN_CLEAR) == LOW) {
    Serial.println("CLEAR → FULL RESET");

    // RX task öldür
    if (rxTaskHandle) {
        vTaskDelete(rxTaskHandle);
        rxTaskHandle = NULL;
    }

    // RX kapat
    rmt_rx_stop(RX_CH);
    rmt_driver_uninstall(RX_CH);

    // STATE RESET
    frame_ready = false;
    replay_count = 0;

    // RX yeniden kur
    ir_rx_init();

    // RX task yeniden başlat
    xTaskCreate(rx_task, "rx_task", 4096, NULL, 10, &rxTaskHandle);

    delay(500);
}


    /* REPLAY */
    if (frame_ready && digitalRead(BTN_REPLAY) == LOW) {
        Serial.println("REPLAY");

        // TSOP KAPAT
        digitalWrite((int)TSOP_PWR, LOW);
        delayMicroseconds(300);

        rmt_rx_stop(RX_CH);

        rmt_write_items(TX_CH, replay_items, replay_count, true);
        rmt_wait_tx_done(TX_CH, portMAX_DELAY);

        delayMicroseconds(300);
        digitalWrite((int)TSOP_PWR, HIGH);

        rmt_rx_start(RX_CH, true);
        digitalWrite(Led_sent, HIGH);
        delay(500);
        digitalWrite(Led_sent, LOW);
    }
}
