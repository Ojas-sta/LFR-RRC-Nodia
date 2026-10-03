#include <Arduino.h>
#include "config.h"
#include "types.h"
#include "sync.h"
#include "nvs_manager.h"
#include "lfr_core.h"
#include "web_server.h"

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n==========================================");
    Serial.println("  LFR-RRC-Nodia Dual-Core Firmware Boot");
    Serial.println("==========================================");

    // 1. Initialize Inter-Core Synchronization Primitives
    Sync::init();

    // 2. Load Configuration and Calibration from NVS
    LFRConfig initialConfig;
    bool hasCalibration = NVSManager::loadConfig(initialConfig);
    Sync::setFullConfig(initialConfig);

    if (hasCalibration) {
        Sync::setStatusMessage("Calibrated & Ready. Place on line and press Start.");
    } else {
        Sync::setStatusMessage("Not calibrated. Put line under sensor array and press Calibrate.");
    }

    // 3. Spawn Core 0 Task: Wi-Fi SoftAP, Web Server & Configuration (Priority 1)
    xTaskCreatePinnedToCore(
        LFRWeb::taskEntry,
        "WebCore0",
        8192,
        NULL,
        1,
        NULL,
        0 // Core 0
    );

    // 4. Spawn Core 1 Task: Real-Time 1250 Hz LFR Control Loop (Priority 24)
    xTaskCreatePinnedToCore(
        lfr_task_entry,
        "LFRCore1",
        8192,
        NULL,
        24, // High priority for deterministic timing
        NULL,
        1 // Core 1
    );

    Serial.println("[Boot] Both cores spawned successfully.");
}

void loop() {
    // Arduino loop task yields indefinitely; Core 0 and Core 1 tasks handle execution
    vTaskDelay(pdMS_TO_TICKS(1000));
}
