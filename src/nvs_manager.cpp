#include "nvs_manager.h"

Preferences NVSManager::prefs;

bool NVSManager::init() {
    return prefs.begin(NVS_NAMESPACE, false);
}

bool NVSManager::loadConfig(LFRConfig &cfg) {
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        Serial.println("[NVS] Failed to open Preferences namespace. Using compile-time defaults.");
        cfg.setFactoryTuningDefaults();
        cfg.resetCalibration();
        return false;
    }

    uint32_t magic = prefs.getUInt("magic", 0);
    uint16_t ver   = prefs.getUShort("version", 0);

    if (magic != CONFIG_MAGIC || ver != CONFIG_VERSION) {
        Serial.println("[NVS] No valid configuration found or version mismatch. Initializing defaults.");
        cfg.setFactoryTuningDefaults();
        cfg.resetCalibration();
        prefs.end();
        return false;
    }

    // Load tuning parameters with fallback to defaults
    cfg.Kp         = prefs.getFloat("kp", DEFAULT_KP);
    cfg.Ki         = prefs.getFloat("ki", DEFAULT_KI);
    cfg.Kd         = prefs.getFloat("kd", DEFAULT_KD);
    cfg.lfSpeed    = prefs.getInt("lf_spd", DEFAULT_LF_SPEED);
    cfg.turnSpeed  = prefs.getInt("turn_spd", DEFAULT_TURN_SPEED);
    cfg.startSpeed = prefs.getInt("start_spd", DEFAULT_START_SPEED);

    // Constrain loaded values to safe ranges
    cfg.Kp         = constrain(cfg.Kp, KP_MIN, KP_MAX);
    cfg.Ki         = constrain(cfg.Ki, KI_MIN, KI_MAX);
    cfg.Kd         = constrain(cfg.Kd, KD_MIN, KD_MAX);
    cfg.lfSpeed    = constrain(cfg.lfSpeed, SPEED_MIN, SPEED_MAX);
    cfg.turnSpeed  = constrain(cfg.turnSpeed, SPEED_MIN, SPEED_MAX);
    cfg.startSpeed = constrain(cfg.startSpeed, SPEED_MIN, SPEED_MAX);

    // Check calibration state
    bool isCal = prefs.getBool("calibrated", false);
    if (!isCal) {
        Serial.println("[NVS] Tuning parameters loaded, but robot is uncalibrated.");
        cfg.resetCalibration();
        prefs.end();
        return false;
    }

    // Read calibration arrays
    size_t minBytes = prefs.getBytes("min_val", cfg.minValues, sizeof(cfg.minValues));
    size_t maxBytes = prefs.getBytes("max_val", cfg.maxValues, sizeof(cfg.maxValues));
    size_t valBytes = prefs.getBytes("valid",   cfg.valid,     sizeof(cfg.valid));
    cfg.lineHigh    = prefs.getBool("line_high", true);

    if (minBytes != sizeof(cfg.minValues) || maxBytes != sizeof(cfg.maxValues) || valBytes != sizeof(cfg.valid)) {
        Serial.println("[NVS] Calibration array size mismatch. Resetting calibration.");
        cfg.resetCalibration();
        prefs.end();
        return false;
    }

    // Verify minimum valid sensors
    int okCount = 0;
    for (int i = 0; i < NUM_SENSORS; i++) {
        if (cfg.valid[i]) okCount++;
    }

    if (okCount < MIN_VALID) {
        Serial.printf("[NVS] Corrupted calibration data (only %d valid sensors). Resetting.\n", okCount);
        cfg.resetCalibration();
        prefs.end();
        return false;
    }

    cfg.calibrated = true;
    Serial.printf("[NVS] Successfully loaded configuration & calibration (%d/%d valid sensors, line reads %s).\n",
                  okCount, NUM_SENSORS, cfg.lineHigh ? "HIGH" : "LOW");
    prefs.end();
    return true;
}

bool NVSManager::saveConfig(const LFRConfig &cfg) {
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        Serial.println("[NVS] Failed to open Preferences for writing.");
        return false;
    }

    prefs.putUInt("magic", CONFIG_MAGIC);
    prefs.putUShort("version", CONFIG_VERSION);

    prefs.putFloat("kp", cfg.Kp);
    prefs.putFloat("ki", cfg.Ki);
    prefs.putFloat("kd", cfg.Kd);
    prefs.putInt("lf_spd", cfg.lfSpeed);
    prefs.putInt("turn_spd", cfg.turnSpeed);
    prefs.putInt("start_spd", cfg.startSpeed);

    prefs.putBool("calibrated", cfg.calibrated);
    prefs.putBool("line_high", cfg.lineHigh);

    if (cfg.calibrated) {
        prefs.putBytes("min_val", cfg.minValues, sizeof(cfg.minValues));
        prefs.putBytes("max_val", cfg.maxValues, sizeof(cfg.maxValues));
        prefs.putBytes("valid",   cfg.valid,     sizeof(cfg.valid));
    }

    prefs.end();
    Serial.println("[NVS] Configuration successfully saved to flash.");
    return true;
}

bool NVSManager::loadDefaults(LFRConfig &cfg) {
    cfg.setFactoryTuningDefaults();
    Serial.println("[NVS] Restored factory tuning defaults in RAM.");
    return true;
}

bool NVSManager::resetCalibration(LFRConfig &cfg) {
    cfg.resetCalibration();

    if (prefs.begin(NVS_NAMESPACE, false)) {
        prefs.putBool("calibrated", false);
        prefs.remove("min_val");
        prefs.remove("max_val");
        prefs.remove("valid");
        prefs.end();
        Serial.println("[NVS] Calibration cleared from flash.");
        return true;
    }
    return false;
}
