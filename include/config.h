#pragma once

#include <Arduino.h>

// ============================================================================
// HARDWARE PIN DEFINITIONS (ESP32 v8 Pinout)
// ============================================================================

// 16-Channel Analog Multiplexer (CD74HC4067 or equivalent)
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
// SENSOR ARRAY & LINE-FOLLOWING CONSTANTS
// ============================================================================
constexpr int   NUM_SENSORS        = 16;
constexpr int   SEEN_THRESHOLD     = 500;   // sensorValue > 500 = on line (range 0..1000)
constexpr int   LOST_FWD           = 255;   // Line-lost recovery fast wheel speed
constexpr int   LOST_REV           = -100;  // Line-lost recovery reverse wheel speed
constexpr int   PWM_MIN            = -100;  // Clamped motor PWM min
constexpr int   PWM_MAX            = 255;   // Clamped motor PWM max

// Sensor geometric weighting: Center = 0, Left = positive, Right = negative
constexpr int   SENSOR_WEIGHTS[16] = { 7, 6, 5, 4, 3, 2, 1, 0, 0, -1, -2, -3, -4, -5, -6, -7 };

// Timing & ADC settings
constexpr uint32_t LOOP_PERIOD_US  = 800;   // ~1250 Hz control loop frequency
constexpr int      MUX_SETTLE_US   = 10;    // Multiplexer channel switch settling time
constexpr int      ADC_SAMPLES     = 2;     // ADC oversampling factor per channel
constexpr uint32_t PWM_FREQ        = 20000; // 20 kHz ultrasonic motor PWM
constexpr uint8_t  PWM_RES         = 8;     // 8-bit resolution (0..255)

// Calibration parameters
constexpr bool     AUTO_POLARITY   = true;  // Detect black/white line automatically
constexpr bool     IS_BLACK_LINE   = true;  // Fallback if AUTO_POLARITY is false
constexpr int      CAL_SPEED       = 90;    // Calibration spin PWM
constexpr uint32_t CAL_TIME_MS     = 5000;  // Total calibration spin time (2.5s each dir)
constexpr int      MIN_RANGE       = 150;   // Minimum (max-min) ADC span to consider sensor valid
constexpr int      MIN_VALID       = 8;     // Minimum working sensors required for valid cal

// ============================================================================
// DEFAULT TUNING PARAMETERS (Can be tuned live and saved to NVS)
// ============================================================================
constexpr float    DEFAULT_KP          = 0.10f;
constexpr float    DEFAULT_KI          = 0.00f;
constexpr float    DEFAULT_KD          = 1.00f;
constexpr int      DEFAULT_LF_SPEED    = 230;   // Top speed on straight lines
constexpr int      DEFAULT_START_SPEED = 180;   // Initial launch speed
constexpr int      DEFAULT_TURN_SPEED  = 80;    // Slower speed during sharp turns

// Slider constraint ranges
constexpr float    KP_MIN              = 0.00f;
constexpr float    KP_MAX              = 5.00f;
constexpr float    KI_MIN              = 0.00f;
constexpr float    KI_MAX              = 1.00f;
constexpr float    KD_MIN              = 0.00f;
constexpr float    KD_MAX              = 20.00f;
constexpr int      SPEED_MIN           = 0;
constexpr int      SPEED_MAX           = 255;

// ============================================================================
// WI-FI AP CONFIGURATION
// ============================================================================
inline const char* AP_SSID             = "LineFollower_AP";
inline const char* AP_PASSWORD         = "linefollow123";
constexpr int      AP_CHANNEL          = 1;
constexpr int      AP_MAX_CONN         = 3;

// ============================================================================
// NVS STORAGE IDENTIFIERS
// ============================================================================
inline const char* NVS_NAMESPACE       = "lfr_cfg";
constexpr uint32_t CONFIG_MAGIC        = 0x4C465231; // ASCII "LFR1"
constexpr uint16_t CONFIG_VERSION      = 1;
