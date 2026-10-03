#include "sync.h"
#include <string.h>

namespace Sync {

    // Synchronization primitives
    static portMUX_TYPE configMux = portMUX_INITIALIZER_UNLOCKED;
    static portMUX_TYPE telemMux  = portMUX_INITIALIZER_UNLOCKED;
    static portMUX_TYPE msgMux    = portMUX_INITIALIZER_UNLOCKED;

    // Command Flags (Core 0 / Buttons -> Core 1)
    static std::atomic<bool> calReq{false};
    static std::atomic<bool> startReq{false};
    static std::atomic<bool> stopReq{false};
    static std::atomic<bool> toggleReq{false};
    static std::atomic<bool> resetCalReq{false};

    // Configuration Handoff
    static std::atomic<bool> configPending{false};
    static LFRConfig         sharedConfig;

    // Telemetry Snapshot
    static LFRTelemetry      sharedTelemetry;

    // Status Message Buffer
    static char              statusMsg[160] = "Initializing...";

    void init() {
        calReq.store(false);
        startReq.store(false);
        stopReq.store(false);
        toggleReq.store(false);
        resetCalReq.store(false);
        configPending.store(false);
    }

    void requestCalibrate() {
        calReq.store(true, std::memory_order_release);
    }

    void requestStart() {
        startReq.store(true, std::memory_order_release);
    }

    void requestStop() {
        stopReq.store(true, std::memory_order_release);
    }

    void requestStartStopToggle() {
        toggleReq.store(true, std::memory_order_release);
    }

    void requestResetCalibration() {
        resetCalReq.store(true, std::memory_order_release);
    }

    bool consumeCalRequest() {
        return calReq.exchange(false, std::memory_order_acq_rel);
    }

    bool consumeStartRequest() {
        return startReq.exchange(false, std::memory_order_acq_rel);
    }

    bool consumeStopRequest() {
        return stopReq.exchange(false, std::memory_order_acq_rel);
    }

    bool consumeStartStopToggleRequest() {
        return toggleReq.exchange(false, std::memory_order_acq_rel);
    }

    bool consumeResetCalRequest() {
        return resetCalReq.exchange(false, std::memory_order_acq_rel);
    }

    void updateTuningConfig(float kp, float ki, float kd, int lfSpeed, int turnSpeed, int startSpeed) {
        portENTER_CRITICAL(&configMux);
        sharedConfig.Kp         = kp;
        sharedConfig.Ki         = ki;
        sharedConfig.Kd         = kd;
        sharedConfig.lfSpeed    = lfSpeed;
        sharedConfig.turnSpeed  = turnSpeed;
        sharedConfig.startSpeed = startSpeed;
        configPending.store(true, std::memory_order_release);
        portEXIT_CRITICAL(&configMux);
    }

    void setFullConfig(const LFRConfig &cfg) {
        portENTER_CRITICAL(&configMux);
        sharedConfig = cfg;
        configPending.store(true, std::memory_order_release);
        portEXIT_CRITICAL(&configMux);
    }

    bool checkConfigUpdate(LFRConfig &outConfig) {
        if (!configPending.load(std::memory_order_acquire)) {
            return false;
        }
        portENTER_CRITICAL(&configMux);
        outConfig = sharedConfig;
        configPending.store(false, std::memory_order_release);
        portEXIT_CRITICAL(&configMux);
        return true;
    }

    void publishCalibrationResult(const int minValues[NUM_SENSORS],
                                  const int maxValues[NUM_SENSORS],
                                  const bool valid[NUM_SENSORS],
                                  bool lineHigh,
                                  bool calibrated) {
        portENTER_CRITICAL(&configMux);
        for (int i = 0; i < NUM_SENSORS; i++) {
            sharedConfig.minValues[i] = minValues[i];
            sharedConfig.maxValues[i] = maxValues[i];
            sharedConfig.valid[i]     = valid[i];
        }
        sharedConfig.lineHigh   = lineHigh;
        sharedConfig.calibrated = calibrated;
        portEXIT_CRITICAL(&configMux);
    }

    void getConfigSnapshot(LFRConfig &outConfig) {
        portENTER_CRITICAL(&configMux);
        outConfig = sharedConfig;
        portEXIT_CRITICAL(&configMux);
    }

    void publishTelemetry(const LFRTelemetry &telem) {
        portENTER_CRITICAL(&telemMux);
        sharedTelemetry = telem;
        portEXIT_CRITICAL(&telemMux);
    }

    void getTelemetrySnapshot(LFRTelemetry &dest) {
        portENTER_CRITICAL(&telemMux);
        dest = sharedTelemetry;
        portEXIT_CRITICAL(&telemMux);
    }

    void setStatusMessage(const char* msg) {
        portENTER_CRITICAL(&msgMux);
        strlcpy(statusMsg, msg, sizeof(statusMsg));
        portEXIT_CRITICAL(&msgMux);
        Serial.println(msg);
    }

    void getStatusMessage(char* dest, size_t maxLen) {
        portENTER_CRITICAL(&msgMux);
        strlcpy(dest, statusMsg, maxLen);
        portEXIT_CRITICAL(&msgMux);
    }

} // namespace Sync
