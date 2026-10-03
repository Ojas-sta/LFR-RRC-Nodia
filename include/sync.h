#pragma once

#ifndef SYNC_H
#define SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Plain C Inter-Core Synchronization API (consumed by Core 1 raw-C core)
void sync_init(void);

void sync_request_calibrate(void);
void sync_request_start(void);
void sync_request_stop(void);
void sync_request_toggle(void);
void sync_request_reset_calibration(void);

bool sync_consume_cal_request(void);
bool sync_consume_start_request(void);
bool sync_consume_stop_request(void);
bool sync_consume_toggle_request(void);
bool sync_consume_reset_cal_request(void);

bool sync_check_config_update(LFRConfig *outConfig);
void sync_publish_calibration_result(const int minValues[NUM_SENSORS],
                                     const int maxValues[NUM_SENSORS],
                                     const bool valid[NUM_SENSORS],
                                     bool lineHigh,
                                     bool calibrated);
void sync_publish_telemetry(const LFRTelemetry *telem);
void sync_set_status_message(const char *msg);

#ifdef __cplusplus
}

// C++ API for Core 0 (WebServer / Main / NVS)
namespace Sync {
    inline void init() { sync_init(); }

    inline void requestCalibrate() { sync_request_calibrate(); }
    inline void requestStart() { sync_request_start(); }
    inline void requestStop() { sync_request_stop(); }
    inline void requestStartStopToggle() { sync_request_toggle(); }
    inline void requestResetCalibration() { sync_request_reset_calibration(); }

    inline bool consumeCalRequest() { return sync_consume_cal_request(); }
    inline bool consumeStartRequest() { return sync_consume_start_request(); }
    inline bool consumeStopRequest() { return sync_consume_stop_request(); }
    inline bool consumeStartStopToggleRequest() { return sync_consume_toggle_request(); }
    inline bool consumeResetCalRequest() { return sync_consume_reset_cal_request(); }

    void updateTuningConfig(float kp, float ki, float kd, int lfSpeed, int turnSpeed, int startSpeed);
    void setFullConfig(const LFRConfig &cfg);
    inline bool checkConfigUpdate(LFRConfig &outConfig) { return sync_check_config_update(&outConfig); }

    inline void publishCalibrationResult(const int minValues[NUM_SENSORS],
                                         const int maxValues[NUM_SENSORS],
                                         const bool valid[NUM_SENSORS],
                                         bool lineHigh,
                                         bool calibrated) {
        sync_publish_calibration_result(minValues, maxValues, valid, lineHigh, calibrated);
    }

    void getConfigSnapshot(LFRConfig &outConfig);
    inline void publishTelemetry(const LFRTelemetry &telem) { sync_publish_telemetry(&telem); }
    void getTelemetrySnapshot(LFRTelemetry &dest);

    inline void setStatusMessage(const char* msg) { sync_set_status_message(msg); }
    void getStatusMessage(char* dest, size_t maxLen);
}
#endif

#endif // SYNC_H
