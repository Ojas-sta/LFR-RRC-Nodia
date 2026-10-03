#include "sync.h"
#include <Arduino.h>
#include <atomic>
#include <string.h>

// Static synchronization primitives
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

extern "C" {

void sync_init(void) {
    calReq.store(false);
    startReq.store(false);
    stopReq.store(false);
    toggleReq.store(false);
    resetCalReq.store(false);
    configPending.store(false);
}

void sync_request_calibrate(void) {
    calReq.store(true, std::memory_order_release);
}

void sync_request_start(void) {
    startReq.store(true, std::memory_order_release);
}

void sync_request_stop(void) {
    stopReq.store(true, std::memory_order_release);
}

void sync_request_toggle(void) {
    toggleReq.store(true, std::memory_order_release);
}

void sync_request_reset_calibration(void) {
    resetCalReq.store(true, std::memory_order_release);
}

bool sync_consume_cal_request(void) {
    return calReq.exchange(false, std::memory_order_acq_rel);
}

bool sync_consume_start_request(void) {
    return startReq.exchange(false, std::memory_order_acq_rel);
}

bool sync_consume_stop_request(void) {
    return stopReq.exchange(false, std::memory_order_acq_rel);
}

bool sync_consume_toggle_request(void) {
    return toggleReq.exchange(false, std::memory_order_acq_rel);
}

bool sync_consume_reset_cal_request(void) {
    return resetCalReq.exchange(false, std::memory_order_acq_rel);
}

bool sync_check_config_update(LFRConfig *outConfig) {
    if (!configPending.load(std::memory_order_acquire)) {
        return false;
    }
    portENTER_CRITICAL(&configMux);
    if (outConfig) {
        *outConfig = sharedConfig;
    }
    configPending.store(false, std::memory_order_release);
    portEXIT_CRITICAL(&configMux);
    return true;
}

void sync_publish_calibration_result(const int minValues[NUM_SENSORS],
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

void sync_publish_telemetry(const LFRTelemetry *telem) {
    if (!telem) return;
    portENTER_CRITICAL(&telemMux);
    sharedTelemetry = *telem;
    portEXIT_CRITICAL(&telemMux);
}

void sync_set_status_message(const char *msg) {
    if (!msg) return;
    portENTER_CRITICAL(&msgMux);
    strlcpy(statusMsg, msg, sizeof(statusMsg));
    portEXIT_CRITICAL(&msgMux);
    Serial.println(msg);
}

} // extern "C"

namespace Sync {

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

    void getConfigSnapshot(LFRConfig &outConfig) {
        portENTER_CRITICAL(&configMux);
        outConfig = sharedConfig;
        portEXIT_CRITICAL(&configMux);
    }

    void getTelemetrySnapshot(LFRTelemetry &dest) {
        portENTER_CRITICAL(&telemMux);
        dest = sharedTelemetry;
        portEXIT_CRITICAL(&telemMux);
    }

    void getStatusMessage(char* dest, size_t maxLen) {
        portENTER_CRITICAL(&msgMux);
        strlcpy(dest, statusMsg, maxLen);
        portEXIT_CRITICAL(&msgMux);
    }

} // namespace Sync
