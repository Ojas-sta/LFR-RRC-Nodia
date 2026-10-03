#pragma once

#include <Arduino.h>
#include "config.h"

// Explicit Robot States
enum class RobotState : uint8_t {
    IDLE = 0,         // Uncalibrated or calibration failed; safe state
    CALIBRATING = 1,  // Spinning left/right across line; gathering calibration data
    READY = 2,        // Calibrated and stationary; waiting for Start command
    RUNNING = 3       // Active real-time line following at ~1250 Hz
};

inline const char* robotStateToString(RobotState s) {
    switch (s) {
        case RobotState::IDLE:        return "IDLE";
        case RobotState::CALIBRATING: return "CALIBRATING";
        case RobotState::READY:       return "READY";
        case RobotState::RUNNING:     return "RUNNING";
        default:                      return "UNKNOWN";
    }
}

// Persistent and Live Configuration
struct LFRConfig {
    // PID Parameters
    float Kp = DEFAULT_KP;
    float Ki = DEFAULT_KI;
    float Kd = DEFAULT_KD;

    // Speed Settings (PWM 0..255)
    int lfSpeed    = DEFAULT_LF_SPEED;
    int turnSpeed  = DEFAULT_TURN_SPEED;
    int startSpeed = DEFAULT_START_SPEED;

    // Calibration Arrays & Status
    int  minValues[NUM_SENSORS] = {0};
    int  maxValues[NUM_SENSORS] = {4095};
    bool valid[NUM_SENSORS]     = {false};
    bool lineHigh               = true;
    bool calibrated             = false;

    // Reset PID and Speeds to compile-time factory defaults
    void setFactoryTuningDefaults() {
        Kp         = DEFAULT_KP;
        Ki         = DEFAULT_KI;
        Kd         = DEFAULT_KD;
        lfSpeed    = DEFAULT_LF_SPEED;
        turnSpeed  = DEFAULT_TURN_SPEED;
        startSpeed = DEFAULT_START_SPEED;
    }

    // Reset calibration tables to blank state
    void resetCalibration() {
        calibrated = false;
        lineHigh   = true;
        for (int i = 0; i < NUM_SENSORS; i++) {
            minValues[i] = 0;
            maxValues[i] = 4095;
            valid[i]     = false;
        }
    }
};

// Telemetry Snapshot published by Core 1 for Core 0 Web Server
struct LFRTelemetry {
    RobotState state = RobotState::IDLE;
    int        calPct = 0;

    int        sensorValue[NUM_SENSORS] = {0};
    bool       valid[NUM_SENSORS]       = {false};

    float      Kp = DEFAULT_KP;
    float      Ki = DEFAULT_KI;
    float      Kd = DEFAULT_KD;
    int        lfSpeed = DEFAULT_LF_SPEED;
    int        turnSpeed = DEFAULT_TURN_SPEED;
    int        startSpeed = DEFAULT_START_SPEED;

    int        lsp = 0;
    int        rsp = 0;
    int        currentSpeed = DEFAULT_START_SPEED;
    double     error = 0.0;
    int        PIDvalue = 0;
    int        onLine = 0;
};
