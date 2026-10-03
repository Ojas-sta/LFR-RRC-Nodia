#include "lfr_core.h"
#include "lfr_bsp.h"
#include "lfr_sensor.h"
#include "lfr_motor.h"
#include "lfr_pid.h"
#include "sync.h"
#include <stdio.h>
#include <string.h>

// Core 1 private state variables
static RobotState currentState = ROBOT_STATE_IDLE;
static int        currentCalPct = 0;
static LFRConfig  activeConfig;

static uint32_t   lastStepUs = 0;
static uint32_t   lastTelemTime = 0;
static uint32_t   lastSensorScan = 0;

// High-resolution loop timing instrumentation
static uint32_t   lastExecUs = 0;
static uint32_t   minExecUs = 999999;
static uint32_t   maxExecUs = 0;
static uint32_t   sumExecUs = 0;
static uint32_t   execCount = 0;

void lfr_init(void) {
    // 1. Initialize hardware peripherals (GPIO, ADC, PWM/LEDC)
    lfr_bsp_init();

    // 2. Fetch initial configuration from shared storage
    sync_check_config_update(&activeConfig);

    if (activeConfig.calibrated) {
        currentState = ROBOT_STATE_READY;
    } else {
        currentState = ROBOT_STATE_IDLE;
    }
}

void lfr_start(void) {
    if (currentState != ROBOT_STATE_READY || !activeConfig.calibrated) {
        sync_set_status_message("Calibrate first.");
        return;
    }

    // Reset control state (exact golden reference reset sequence)
    lfr_pid_reset(activeConfig.startSpeed);
    lfr_sensor_set_online(1);

    lastStepUs = lfr_bsp_micros();
    currentState = ROBOT_STATE_RUNNING;
    sync_set_status_message("Running.");
}

void lfr_stop(void) {
    stopMotors();
    lfr_bsp_led_set(false);
    currentState = ROBOT_STATE_READY;
    sync_set_status_message("Stopped. Press Start to run again.");
}

void lfr_step(void) {
    // Deterministic microsecond busy-wait pacing (800 µs / ~1250 Hz)
    while ((uint32_t)(lfr_bsp_micros() - lastStepUs) < LOOP_PERIOD_US) {}
    lastStepUs = lfr_bsp_micros();

    uint32_t t0 = lfr_bsp_micros();

    // 1. Acquire and normalize 16-channel sensor array
    readLine(&activeConfig);

    // 2. Evaluate outer turn sensors and speed ramping
    lfr_handle_sharp_turns_and_speed(&activeConfig);

    // 3. Execution branch: On-line PID vs Off-line recovery
    if (lfr_sensor_get_online() == 1) {
        linefollow(&activeConfig);
        lfr_bsp_led_set(true);
    } else {
        lfr_handle_lost_line(&activeConfig);
    }

    // Record execution time metrics
    uint32_t t_exec = lfr_bsp_micros() - t0;
    lastExecUs = t_exec;
    if (t_exec < minExecUs) minExecUs = t_exec;
    if (t_exec > maxExecUs) maxExecUs = t_exec;
    sumExecUs += t_exec;
    execCount++;
}

void lfr_handle_buttons(void) {
    static bool lastCal = false;
    static bool lastStart = false;
    static uint32_t lastCalTime = 0;
    static uint32_t lastStartTime = 0;

    bool calPressed = lfr_bsp_cal_btn_pressed();
    bool startPressed = lfr_bsp_start_btn_pressed();
    uint32_t now = lfr_bsp_millis();

    if (calPressed && !lastCal && (now - lastCalTime > 300)) {
        lastCalTime = now;
        sync_request_calibrate();
    }
    lastCal = calPressed;

    if (startPressed && !lastStart && (now - lastStartTime > 300)) {
        lastStartTime = now;
        sync_request_toggle();
    }
    lastStart = startPressed;
}

static void publish_telemetry(void) {
    LFRTelemetry telem;
    memset(&telem, 0, sizeof(telem));

    telem.state        = currentState;
    telem.calPct       = currentCalPct;
    telem.Kp           = activeConfig.Kp;
    telem.Ki           = activeConfig.Ki;
    telem.Kd           = activeConfig.Kd;
    telem.lfSpeed      = activeConfig.lfSpeed;
    telem.turnSpeed    = activeConfig.turnSpeed;
    telem.startSpeed   = activeConfig.startSpeed;
    telem.lsp          = lfr_pid_get_lsp();
    telem.rsp          = lfr_pid_get_rsp();
    telem.currentSpeed = lfr_pid_get_current_speed();
    telem.error        = lfr_pid_get_error();
    telem.PIDvalue     = lfr_pid_get_pidvalue();
    telem.onLine       = lfr_sensor_get_online();

    const int *sensorValues = lfr_sensor_get_values();
    for (int i = 0; i < NUM_SENSORS; i++) {
        telem.sensorValue[i] = sensorValues[i];
        telem.valid[i]       = activeConfig.valid[i];
    }

    telem.loopExecUs = lastExecUs;
    telem.minExecUs  = (minExecUs == 999999) ? 0 : minExecUs;
    telem.maxExecUs  = maxExecUs;

    sync_publish_telemetry(&telem);
}

void lfr_task_entry(void *parameter) {
    (void)parameter;
    lfr_init();
    printf("[Core 1] Raw-C real-time LFR engine started on Core 1.\n");

    for (;;) {
        // 1. Process physical button inputs
        lfr_handle_buttons();

        // 2. Fetch live tuning updates from Core 0
        sync_check_config_update(&activeConfig);

        // 3. Process Inter-Core Command Requests
        if (sync_consume_reset_cal_request()) {
            if (currentState == ROBOT_STATE_RUNNING) {
                lfr_stop();
            }
            lfr_config_reset_cal(&activeConfig);
            currentState = ROBOT_STATE_IDLE;
            sync_set_status_message("Calibration reset. Please calibrate before starting.");
        }

        if (sync_consume_cal_request()) {
            if (currentState == ROBOT_STATE_RUNNING) {
                sync_set_status_message("Stop first to calibrate.");
            } else {
                currentState = ROBOT_STATE_CALIBRATING;
                sync_set_status_message("Calibrating... robot will spin left and right over the line.");
                lfr_bsp_delay_ms(1000); // 1-second safety pause before spin

                bool ok = calibrate(&activeConfig, &currentCalPct);
                if (ok) {
                    currentState = ROBOT_STATE_READY;
                    char msg[160];
                    snprintf(msg, sizeof(msg),
                             "Calibration OK: line reads %s. Place robot on the line and press Start.",
                             activeConfig.lineHigh ? "HIGH" : "LOW");
                    sync_set_status_message(msg);
                    sync_publish_calibration_result(activeConfig.minValues,
                                                    activeConfig.maxValues,
                                                    activeConfig.valid,
                                                    activeConfig.lineHigh,
                                                    true);
                } else {
                    currentState = ROBOT_STATE_IDLE;
                    sync_set_status_message("Calibration FAILED: insufficient contrast. Check surface and spin.");
                    sync_publish_calibration_result(activeConfig.minValues,
                                                    activeConfig.maxValues,
                                                    activeConfig.valid,
                                                    activeConfig.lineHigh,
                                                    false);
                }
            }
        }

        if (sync_consume_start_request()) {
            if (currentState != ROBOT_STATE_RUNNING) {
                lfr_start();
            }
        }

        if (sync_consume_stop_request()) {
            if (currentState == ROBOT_STATE_RUNNING) {
                lfr_stop();
            }
        }

        if (sync_consume_toggle_request()) {
            if (currentState == ROBOT_STATE_RUNNING) {
                lfr_stop();
            } else {
                lfr_start();
            }
        }

        // 4. State Execution
        if (currentState == ROBOT_STATE_RUNNING) {
            lfr_step();
        } else {
            // Idle/Ready stationary behavior
            if (currentState == ROBOT_STATE_IDLE) {
                lfr_bsp_led_set(((lfr_bsp_millis() / 250) & 1) != 0); // 250ms blink = uncalibrated
            }

            // Stationary sensor scan at 20 Hz for live Web UI visualization
            if (activeConfig.calibrated && (lfr_bsp_millis() - lastSensorScan > 50)) {
                lastSensorScan = lfr_bsp_millis();
                readLine(&activeConfig);
            }

            lfr_bsp_delay_ms(1);
        }

        // 5. Periodic Telemetry Publish (~20 Hz)
        if (lfr_bsp_millis() - lastTelemTime > 50) {
            lastTelemTime = lfr_bsp_millis();
            publish_telemetry();
        }
    }
}

uint32_t lfr_get_last_exec_us(void) {
    return lastExecUs;
}

uint32_t lfr_get_min_exec_us(void) {
    return (minExecUs == 999999) ? 0 : minExecUs;
}

uint32_t lfr_get_max_exec_us(void) {
    return maxExecUs;
}

void lfr_config_init_defaults(LFRConfig *cfg) {
    if (!cfg) return;
    cfg->Kp         = DEFAULT_KP;
    cfg->Ki         = DEFAULT_KI;
    cfg->Kd         = DEFAULT_KD;
    cfg->lfSpeed    = DEFAULT_LF_SPEED;
    cfg->turnSpeed  = DEFAULT_TURN_SPEED;
    cfg->startSpeed = DEFAULT_START_SPEED;
}

void lfr_config_reset_cal(LFRConfig *cfg) {
    if (!cfg) return;
    cfg->calibrated = false;
    cfg->lineHigh   = true;
    for (int i = 0; i < NUM_SENSORS; i++) {
        cfg->minValues[i] = 0;
        cfg->maxValues[i] = 4095;
        cfg->valid[i]     = false;
    }
}
