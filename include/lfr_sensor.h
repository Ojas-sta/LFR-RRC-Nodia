#pragma once

#ifndef LFR_SENSOR_H
#define LFR_SENSOR_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Sensor acquisition primitives (exact golden reference names and signatures)
int  sensorRead(int channel);
void readAll(void);
void readLine(const LFRConfig *cfg);

// Calibration routines
int  med3(int a, int b, int c);
bool calibrate(LFRConfig *cfg, int *calPctOut);

// Buffer accessors for PID, telemetry, and decision logic
const int* lfr_sensor_get_raw(void);
const int* lfr_sensor_get_values(void);
const int* lfr_sensor_get_array(void);
int        lfr_sensor_get_online(void);
void       lfr_sensor_set_online(int onLineVal);

#ifdef __cplusplus
}
#endif

#endif // LFR_SENSOR_H
