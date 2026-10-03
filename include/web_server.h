#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "types.h"

namespace LFRWeb {

    // Initialize Wi-Fi SoftAP and WebServer endpoints
    void init();

    // FreeRTOS task entry point pinned to Core 0
    void taskEntry(void* parameter);

} // namespace LFRWeb
