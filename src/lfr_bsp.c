#include "lfr_bsp.h"
#include <Arduino.h>
#include <esp_timer.h>

void lfr_bsp_init(void) {
    // 1. Multiplexer control GPIOs
    pinMode(MUX_S0, OUTPUT);
    pinMode(MUX_S1, OUTPUT);
    pinMode(MUX_S2, OUTPUT);
    pinMode(MUX_S3, OUTPUT);
    pinMode(MUX_EN, OUTPUT);
    digitalWrite(MUX_EN, LOW); // Active LOW -> enable multiplexer
    pinMode(MUX_SIG, INPUT);

    // 2. TB6612FNG motor driver control GPIOs
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(BIN1, OUTPUT);
    pinMode(BIN2, OUTPUT);
    pinMode(STBY, OUTPUT);
    digitalWrite(STBY, HIGH);  // Active HIGH -> enable H-bridges

    // 3. User Interface: Buttons & LED
    pinMode(CAL_BTN_PIN, INPUT_PULLUP);
    pinMode(START_BTN_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // 4. Ultrasonic Motor PWM (20 kHz, 8-bit resolution 0..255)
    ledcAttach(PWMA, PWM_FREQ, PWM_RES);
    ledcAttach(PWMB, PWM_FREQ, PWM_RES);
    lfr_bsp_motors_stop();

    // 5. ADC Configuration (12-bit, 0..3.3V range with 11dB attenuation)
    analogReadResolution(12);
    analogSetPinAttenuation(MUX_SIG, ADC_11db);
}

void lfr_bsp_mux_select(uint8_t channel) {
    digitalWrite(MUX_S0, (channel >> 0) & 1);
    digitalWrite(MUX_S1, (channel >> 1) & 1);
    digitalWrite(MUX_S2, (channel >> 2) & 1);
    digitalWrite(MUX_S3, (channel >> 3) & 1);
    delayMicroseconds(MUX_SETTLE_US);
}

uint16_t lfr_bsp_adc_read(void) {
    int s = 0;
    for (int k = 0; k < ADC_SAMPLES; k++) {
        s += analogRead(MUX_SIG);
    }
    return (uint16_t)(s / ADC_SAMPLES);
}

void lfr_bsp_motor1_set(int motorSpeed) {
    if (motorSpeed > 255) motorSpeed = 255;
    if (motorSpeed < -255) motorSpeed = -255;

    if (motorSpeed > 0) {
        digitalWrite(AIN1, HIGH);
        digitalWrite(AIN2, LOW);
        ledcWrite(PWMA, motorSpeed);
    } else if (motorSpeed < 0) {
        digitalWrite(AIN1, LOW);
        digitalWrite(AIN2, HIGH);
        ledcWrite(PWMA, -motorSpeed);
    } else {
        digitalWrite(AIN1, HIGH);
        digitalWrite(AIN2, HIGH);
        ledcWrite(PWMA, 0);
    }
}

void lfr_bsp_motor2_set(int motorSpeed) {
    if (motorSpeed > 255) motorSpeed = 255;
    if (motorSpeed < -255) motorSpeed = -255;

    if (motorSpeed > 0) {
        digitalWrite(BIN1, HIGH);
        digitalWrite(BIN2, LOW);
        ledcWrite(PWMB, motorSpeed);
    } else if (motorSpeed < 0) {
        digitalWrite(BIN1, LOW);
        digitalWrite(BIN2, HIGH);
        ledcWrite(PWMB, -motorSpeed);
    } else {
        digitalWrite(BIN1, HIGH);
        digitalWrite(BIN2, HIGH);
        ledcWrite(PWMB, 0);
    }
}

void lfr_bsp_motors_stop(void) {
    lfr_bsp_motor1_set(0);
    lfr_bsp_motor2_set(0);
}

void lfr_bsp_led_set(bool on) {
    digitalWrite(LED_PIN, on ? HIGH : LOW);
}

bool lfr_bsp_cal_btn_pressed(void) {
    return (digitalRead(CAL_BTN_PIN) == LOW);
}

bool lfr_bsp_start_btn_pressed(void) {
    return (digitalRead(START_BTN_PIN) == LOW);
}

uint32_t lfr_bsp_micros(void) {
    return (uint32_t)esp_timer_get_time();
}

uint32_t lfr_bsp_millis(void) {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

void lfr_bsp_delay_ms(uint32_t ms) {
    delay(ms);
}

void lfr_bsp_delay_us(uint32_t us) {
    delayMicroseconds(us);
}
