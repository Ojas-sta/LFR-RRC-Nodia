#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "types.h"

class NVSManager {
public:
    // Initialize NVS namespace
    static bool init();

    // Load configuration from NVS.
    // Returns true if valid calibration data was found (eligible for READY state).
    // Returns false if calibration is missing or invalid (enters IDLE state).
    static bool loadConfig(LFRConfig &cfg);

    // Save full configuration snapshot (PID, speeds, and calibration) to NVS
    static bool saveConfig(const LFRConfig &cfg);

    // Reset tuning parameters to factory defaults while preserving calibration
    static bool loadDefaults(LFRConfig &cfg);

    // Reset calibration state and delete stored calibration arrays
    static bool resetCalibration(LFRConfig &cfg);

private:
    static Preferences prefs;
};
