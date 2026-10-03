#pragma once

#include <Arduino.h>
#include "config.h"
#include "types.h"

namespace LFRCore {

    // Hardware and Core 1 peripheral initialization
    void initHardware();

    // FreeRTOS task entry point pinned to Core 1
    void taskEntry(void* parameter);

    // Motor driver direct primitives (Core 1 only!)
    void motor1run(int motorSpeed); // Left motor
    void motor2run(int motorSpeed); // Right motor
    void stopMotors();

    // Low-level sensor scanning
    int  sensorRead(int channel);
    void readAllRaw(int rawOut[NUM_SENSORS]);
    void readLine();

    // Calibration and Line-following algorithms
    void calibrate();
    void linefollow();
    void runStep();

    // State transitions
    void startRun();
    void stopRun();

    // Button handling
    void handleButtons();

} // namespace LFRCore
