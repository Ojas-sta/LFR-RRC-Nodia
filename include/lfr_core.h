#pragma once

#ifndef LFR_CORE_H
#define LFR_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Core 1 LFR state machine and lifecycle
void lfr_init(void);
void lfr_start(void);
void lfr_stop(void);
void lfr_step(void);

// FreeRTOS task entry point pinned to Core 1
void lfr_task_entry(void *parameter);

// Physical button handling (GPIO 4 & 5)
void lfr_handle_buttons(void);

// Microsecond timing diagnostics
uint32_t lfr_get_last_exec_us(void);
uint32_t lfr_get_min_exec_us(void);
uint32_t lfr_get_max_exec_us(void);

#ifdef __cplusplus
}
#endif

#endif // LFR_CORE_H
