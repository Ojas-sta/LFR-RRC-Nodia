#pragma once

#ifndef LFR_BSP_H
#define LFR_BSP_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize ESP32 GPIO, ADC, PWM/LEDC, and timer peripherals
void lfr_bsp_init(void);

// Multiplexer channel selection with settling delay
void lfr_bsp_mux_select(uint8_t channel);

// Low-level ADC read with hardware oversampling
uint16_t lfr_bsp_adc_read(void);

// TB6612FNG H-bridge primitives (Motor 1 = Left wheel, Motor 2 = Right wheel)
void lfr_bsp_motor1_set(int speed);
void lfr_bsp_motor2_set(int speed);
void lfr_bsp_motors_stop(void);

// LED output control
void lfr_bsp_led_set(bool on);

// Pushbutton inputs
bool lfr_bsp_cal_btn_pressed(void);
bool lfr_bsp_start_btn_pressed(void);

// High-resolution timing
uint32_t lfr_bsp_micros(void);
uint32_t lfr_bsp_millis(void);
void lfr_bsp_delay_ms(uint32_t ms);
void lfr_bsp_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif // LFR_BSP_H
