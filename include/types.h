#pragma once

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Explicit Robot States (C enum with C++ backward compatibility)
typedef enum {
    ROBOT_STATE_IDLE = 0,        // Uncalibrated or calibration failed; safe state
    ROBOT_STATE_CALIBRATING = 1, // Spinning left/right across line; gathering calibration data
    ROBOT_STATE_READY = 2,       // Calibrated and stationary; waiting for Start command
    ROBOT_STATE_RUNNING = 3      // Active real-time line following at ~1250 Hz
} RobotState;

#define IDLE        ROBOT_STATE_IDLE
#define CALIBRATING ROBOT_STATE_CALIBRATING
#define READY       ROBOT_STATE_READY
#define RUNNING     ROBOT_STATE_RUNNING

static inline const char* robotStateToString(RobotState s) {
    switch (s) {
        case ROBOT_STATE_IDLE:        return "IDLE";
        case ROBOT_STATE_CALIBRATING: return "CALIBRATING";
        case ROBOT_STATE_READY:       return "READY";
        case ROBOT_STATE_RUNNING:     return "RUNNING";
        default:                      return "UNKNOWN";
    }
}

// Persistent and Live Configuration
typedef struct {
    // PID Parameters
    float Kp;
    float Ki;
    float Kd;

    // Speed Settings (PWM 0..255)
    int lfSpeed;
    int turnSpeed;
    int startSpeed;

    // Calibration Arrays & Status
    int  minValues[NUM_SENSORS];
    int  maxValues[NUM_SENSORS];
    bool valid[NUM_SENSORS];
    bool lineHigh;
    bool calibrated;

#ifdef __cplusplus
    // Convenience methods for C++ components (NVSManager / WebServer)
    void setFactoryTuningDefaults() {
        Kp         = DEFAULT_KP;
        Ki         = DEFAULT_KI;
        Kd         = DEFAULT_KD;
        lfSpeed    = DEFAULT_LF_SPEED;
        turnSpeed  = DEFAULT_TURN_SPEED;
        startSpeed = DEFAULT_START_SPEED;
    }

    void resetCalibration() {
        calibrated = false;
        lineHigh   = true;
        for (int i = 0; i < NUM_SENSORS; i++) {
            minValues[i] = 0;
            maxValues[i] = 4095;
            valid[i]     = false;
        }
    }
#endif
} LFRConfig;

// Telemetry Snapshot published by Core 1 for Core 0 Web Server
typedef struct {
    RobotState state;
    int        calPct;

    int        sensorValue[NUM_SENSORS];
    bool       valid[NUM_SENSORS];

    float      Kp;
    float      Ki;
    float      Kd;
    int        lfSpeed;
    int        turnSpeed;
    int        startSpeed;

    int        lsp;
    int        rsp;
    int        currentSpeed;
    double     error;
    int        PIDvalue;
    int        onLine;

    // Timing diagnostics
    uint32_t   loopExecUs;
    uint32_t   minExecUs;
    uint32_t   maxExecUs;
} LFRTelemetry;

// Plain C initialization helpers
void lfr_config_init_defaults(LFRConfig *cfg);
void lfr_config_reset_cal(LFRConfig *cfg);

#ifdef __cplusplus
}
#endif

#endif // TYPES_H
