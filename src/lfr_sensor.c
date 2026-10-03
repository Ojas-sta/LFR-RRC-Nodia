#include "lfr_sensor.h"
#include "lfr_bsp.h"
#include "lfr_motor.h"
#include <stdio.h>

// Static 16-channel sensor buffers (Core 1 private)
static int raw[NUM_SENSORS];
static int sensorValue[NUM_SENSORS];
static int sensorArray[NUM_SENSORS];
static int threshold[NUM_SENSORS];
static int onLine = 1;

int sensorRead(int channel) {
    lfr_bsp_mux_select((uint8_t)channel);
    return (int)lfr_bsp_adc_read();
}

void readAll(void) {
    for (int i = 0; i < NUM_SENSORS; i++) {
        raw[i] = sensorRead(i);
    }
}

void readLine(const LFRConfig *cfg) {
    readAll();
    onLine = 0;
    for (int i = 0; i < NUM_SENSORS; i++) {
        if (!cfg->valid[i]) {
            sensorValue[i] = 0;
            sensorArray[i] = 0;
            continue;
        }
        int span = cfg->maxValues[i] - cfg->minValues[i];
        if (span <= 0) span = 1;
        long v = (long)(raw[i] - cfg->minValues[i]) * 1000L / span;
        if (!cfg->lineHigh) {
            v = 1000 - v;
        }
        if (v < 0) v = 0;
        if (v > 1000) v = 1000;
        sensorValue[i] = (int)v;
        sensorArray[i] = (sensorValue[i] > SEEN_THRESHOLD) ? 1 : 0;
        if (sensorArray[i]) {
            onLine = 1;
        }
    }
}

int med3(int a, int b, int c) {
    if (a > b) { int t = a; a = b; b = t; }
    int m = (b < c) ? b : c;
    return (a > m) ? a : m;
}

bool calibrate(LFRConfig *cfg, int *calPctOut) {
    if (calPctOut) *calPctOut = 0;

    int p1[NUM_SENSORS], p2[NUM_SENSORS];
    uint32_t sum[NUM_SENSORS];
    uint32_t n = 0;

    readAll();
    for (int i = 0; i < NUM_SENSORS; i++) {
        p1[i] = p2[i] = raw[i];
        cfg->minValues[i] = 4095;
        cfg->maxValues[i] = 0;
        sum[i] = 0;
    }

    // Two phases: spin left (2.5s), pause, spin right (2.5s)
    for (int phase = 0; phase < 2; phase++) {
        int dir = (phase == 0) ? 1 : -1;
        uint32_t t0 = lfr_bsp_millis();
        while ((lfr_bsp_millis() - t0) < (CAL_TIME_MS / 2)) {
            motor1run(dir * CAL_SPEED);
            motor2run(-dir * CAL_SPEED);

            readAll();
            for (int i = 0; i < NUM_SENSORS; i++) {
                int m = med3(p2[i], p1[i], raw[i]); // Reject single-sample spikes
                p2[i] = p1[i];
                p1[i] = raw[i];
                if (m < cfg->minValues[i]) cfg->minValues[i] = m;
                if (m > cfg->maxValues[i]) cfg->maxValues[i] = m;
                sum[i] += m;
            }
            n++;

            if (calPctOut) {
                *calPctOut = (int)((phase * (CAL_TIME_MS / 2) + (lfr_bsp_millis() - t0)) * 100 / CAL_TIME_MS);
            }
            lfr_bsp_led_set(((lfr_bsp_millis() / 100) & 1) != 0);
        }
        stopMotors();
        lfr_bsp_delay_ms(150);
    }

    stopMotors();
    lfr_bsp_led_set(false);

    // Evaluate valid sensors & line polarity
    int ok = 0, votesHigh = 0, votesLow = 0, best = 0, clip = 0;
    for (int i = 0; i < NUM_SENSORS; i++) {
        int range = cfg->maxValues[i] - cfg->minValues[i];
        if (range > best) best = range;
        if (cfg->maxValues[i] >= 4050) clip++;
        cfg->valid[i] = (range >= MIN_RANGE);
        if (cfg->valid[i]) {
            ok++;
            float mean = (float)sum[i] / (float)n;
            if ((mean - cfg->minValues[i]) / (float)range < 0.5f) {
                votesHigh++;
            } else {
                votesLow++;
            }
        }
        threshold[i] = (cfg->minValues[i] + cfg->maxValues[i]) / 2;
    }

    cfg->lineHigh = AUTO_POLARITY ? (votesHigh >= votesLow) : (bool)IS_BLACK_LINE;

    // Serial calibration report
    printf("min: "); for (int i = 0; i < NUM_SENSORS; i++) { printf("%d ", cfg->minValues[i]); } printf("\n");
    printf("max: "); for (int i = 0; i < NUM_SENSORS; i++) { printf("%d ", cfg->maxValues[i]); } printf("\n");
    printf("thr: "); for (int i = 0; i < NUM_SENSORS; i++) { printf("%d ", threshold[i]); } printf("\n");
    printf("ok : "); for (int i = 0; i < NUM_SENSORS; i++) { printf("%s ", cfg->valid[i] ? "y" : "-"); } printf("\n");

    if (ok < MIN_VALID) {
        cfg->calibrated = false;
        return false;
    }

    cfg->calibrated = true;
    if (calPctOut) *calPctOut = 100;
    return true;
}

const int* lfr_sensor_get_raw(void) {
    return raw;
}

const int* lfr_sensor_get_values(void) {
    return sensorValue;
}

const int* lfr_sensor_get_array(void) {
    return sensorArray;
}

int lfr_sensor_get_online(void) {
    return onLine;
}

void lfr_sensor_set_online(int onLineVal) {
    onLine = onLineVal;
}
