#pragma once

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// HARDWARE PIN DEFINITIONS (ESP32 v8 Pinout)
// ============================================================================

// 16-Channel Analog Multiplexer (CD74HC4067)
#define MUX_S0          32
#define MUX_S1          33
#define MUX_S2          25
#define MUX_S3          26
#define MUX_SIG         34    // ADC1 Channel 6 (Input only)
#define MUX_EN          27    // Active LOW

// Motor Driver (TB6612FNG)
// Motor 1 = A (Left wheel)
#define AIN1            15
#define AIN2            14
#define PWMA            13
// Motor 2 = B (Right wheel)
#define BIN1            19
#define BIN2            21
#define PWMB            18
// Standby control (Active HIGH to enable motor outputs)
#define STBY            23

// User Interface & Buttons
#define CAL_BTN_PIN      4    // Active LOW (INPUT_PULLUP)
#define START_BTN_PIN    5    // Active LOW (INPUT_PULLUP)
#define LED_PIN          2    // Status LED (Active HIGH)

// ============================================================================
// SENSOR ARRAY & LINE-FOLLOWING CONSTANTS (16-Channel Nano LF-2 Reference)
// ============================================================================
#define NUM_SENSORS        16
#define SEEN_THRESHOLD     500   // sensorValue > 500 = on line (range 0..1000)
#define LOST_FWD           255   // Line-lost recovery fast wheel speed
#define LOST_REV           (-100)// Line-lost recovery reverse wheel speed
#define PWM_MIN            (-100)// Clamped motor PWM min
#define PWM_MAX            255   // Clamped motor PWM max

// Sensor geometric weighting (exact 16-channel array from reference code):
static const int SENSOR_WEIGHTS[NUM_SENSORS] = { 7, 6, 5, 4, 3, 2, 1, 0, 0, -1, -2, -3, -4, -5, -6, -7 };

// Timing & ADC settings
#define LOOP_PERIOD_US     800   // ~1250 Hz control loop frequency (reference Nano LF-2)
#define MUX_SETTLE_US      10    // Multiplexer channel switch settling time (microseconds)
#define ADC_SAMPLES        2     // ADC oversampling factor per channel
#define PWM_FREQ           20000 // 20 kHz ultrasonic motor PWM
#define PWM_RES            8     // 8-bit resolution (0..255)

// Calibration parameters
#define AUTO_POLARITY      true  // Detect black/white line automatically
#define IS_BLACK_LINE      true  // Fallback if AUTO_POLARITY is false
#define CAL_SPEED          90    // Calibration spin PWM
#define CAL_TIME_MS        5000  // Total calibration spin time (2.5s each dir)
#define MIN_RANGE          150   // Minimum (max-min) ADC span to consider sensor valid
#define MIN_VALID          8     // Need at least 8 working sensors (out of 16)

// ============================================================================
// DEFAULT TUNING PARAMETERS (Exact tuned defaults from reference source)
// ============================================================================
#define DEFAULT_KP          0.10f
#define DEFAULT_KI          0.00f
#define DEFAULT_KD          1.00f
#define DEFAULT_LF_SPEED    230   // Top speed on straight lines
#define DEFAULT_START_SPEED 180   // Initial launch speed
#define DEFAULT_TURN_SPEED  80    // Slower speed during sharp turns

// Slider constraint ranges
#define KP_MIN              0.00f
#define KP_MAX              5.00f
#define KI_MIN              0.00f
#define KI_MAX              1.00f
#define KD_MIN              0.00f
#define KD_MAX              20.00f
#define SPEED_MIN           0
#define SPEED_MAX           255

// ============================================================================
// WI-FI AP CONFIGURATION
// ============================================================================
#define AP_SSID             "LineFollower_AP"
#define AP_PASSWORD         "linefollow123"
#define AP_CHANNEL          1
#define AP_MAX_CONN         3

// ============================================================================
// NVS STORAGE IDENTIFIERS
// ============================================================================
#define NVS_NAMESPACE       "lfr_cfg"
#define CONFIG_MAGIC        0x4C465231 // ASCII "LFR1"
#define CONFIG_VERSION      1          // 16-channel layout version

#ifdef __cplusplus
}
#endif

#endif // CONFIG_H
