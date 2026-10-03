#pragma once

#include <Arduino.h>
#include <atomic>
#include "types.h"

namespace Sync {

    // Initialize synchronization primitives
    void init();

    // ---------------- Command Dispatch (Core 0 / Buttons -> Core 1) ----------------
    void requestCalibrate();
    void requestStart();
    void requestStop();
    void requestStartStopToggle();
    void requestResetCalibration();

    // Check & consume commands (called by Core 1 control loop)
    bool consumeCalRequest();
    bool consumeStartRequest();
    bool consumeStopRequest();
    bool consumeStartStopToggleRequest();
    bool consumeResetCalRequest();

    // ---------------- Live Configuration (Core 0 -> Core 1) ----------------
    // Core 0 updates live tuning parameters (PID / speeds)
    void updateTuningConfig(float kp, float ki, float kd, int lfSpeed, int turnSpeed, int startSpeed);
    
    // Core 0 replaces full configuration (e.g. on load defaults or NVS load)
    void setFullConfig(const LFRConfig &cfg);

    // Core 1 checks if a new configuration snapshot is pending (lock-free fast path)
    bool checkConfigUpdate(LFRConfig &outConfig);

    // Core 1 publishes completed calibration data back to shared storage
    void publishCalibrationResult(const int minValues[NUM_SENSORS],
                                  const int maxValues[NUM_SENSORS],
                                  const bool valid[NUM_SENSORS],
                                  bool lineHigh,
                                  bool calibrated);

    // Retrieve current configuration for Core 0 (e.g. for saving to NVS)
    void getConfigSnapshot(LFRConfig &outConfig);

    // ---------------- Telemetry Snapshot (Core 1 -> Core 0) ----------------
    void publishTelemetry(const LFRTelemetry &telem);
    void getTelemetrySnapshot(LFRTelemetry &dest);

    // ---------------- Thread-Safe Status Message ----------------
    void setStatusMessage(const char* msg);
    void getStatusMessage(char* dest, size_t maxLen);

} // namespace Sync
