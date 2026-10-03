#include "lfr_pid.h"
#include "lfr_sensor.h"
#include "lfr_motor.h"
#include "lfr_bsp.h"

// State variables (exact golden reference types and semantics)
static int    P = 0, I = 0, D = 0;
static int    previousError = 0;
static int    PIDvalue = 0;
static double error = 0.0;
static int    lsp = 0, rsp = 0;
static int    currentSpeed = DEFAULT_START_SPEED;
static int    lastTurnDir = 0; // 1 = line last seen on sensor 0 side, -1 = sensor 15 side
static int    activeSensors = 0;

void lfr_pid_reset(int startSpeed) {
    P = I = D = previousError = PIDvalue = 0;
    error = 0.0;
    lsp = rsp = 0;
    lastTurnDir = 0;
    currentSpeed = startSpeed;
}

void linefollow(const LFRConfig *cfg) {
    error = 0.0;
    activeSensors = 0;
    const int *sensorArray = lfr_sensor_get_array();
    const int *sensorValue = lfr_sensor_get_values();

    for (int i = 0; i < NUM_SENSORS; i++) {
        if (sensorArray[i]) {
            error += SENSOR_WEIGHTS[i] * sensorArray[i] * sensorValue[i];
        }
        activeSensors += sensorArray[i];
    }
    // Only called when onLine == 1, so activeSensors >= 1
    error = error / activeSensors;

    // EXACT GOLDEN REFERENCE PID FORMULATION (NO PREMATURE CASTS)
    P = error;
    I = I + error;
    D = error - previousError;

    PIDvalue = (cfg->Kp * P) + (cfg->Ki * I) + (cfg->Kd * D);
    previousError = error;

    lsp = currentSpeed - PIDvalue;
    rsp = currentSpeed + PIDvalue;

    if (lsp < PWM_MIN) lsp = PWM_MIN;
    if (lsp > PWM_MAX) lsp = PWM_MAX;
    if (rsp < PWM_MIN) rsp = PWM_MIN;
    if (rsp > PWM_MAX) rsp = PWM_MAX;

    motor1run(lsp);
    motor2run(rsp);
}

void lfr_handle_sharp_turns_and_speed(const LFRConfig *cfg) {
    const int *sensorArray = lfr_sensor_get_array();

    // Remember which side the line was last seen on
    if (sensorArray[0] || sensorArray[1]) {
        lastTurnDir = 1;
    } else if (sensorArray[14] || sensorArray[15]) {
        lastTurnDir = -1;
    }

    // Outer sensors see the line = sharp turn, so slow down
    if (sensorArray[0] || sensorArray[1] || sensorArray[14] || sensorArray[15]) {
        currentSpeed = cfg->turnSpeed;
    } else if (currentSpeed < cfg->lfSpeed) {
        currentSpeed++; // Ramp back up after the turn
    }
}

void lfr_handle_lost_line(const LFRConfig *cfg) {
    lfr_bsp_led_set(false);
    currentSpeed = cfg->turnSpeed;

    if (lastTurnDir == 1) {
        motor1run(LOST_REV);
        motor2run(LOST_FWD);
    } else if (lastTurnDir == -1) {
        motor1run(LOST_FWD);
        motor2run(LOST_REV);
    } else {
        if (error >= 0.0) {
            motor1run(LOST_REV);
            motor2run(LOST_FWD);
        } else {
            motor1run(LOST_FWD);
            motor2run(LOST_REV);
        }
    }
}

double lfr_pid_get_error(void) {
    return error;
}

int lfr_pid_get_p(void) {
    return P;
}

int lfr_pid_get_i(void) {
    return I;
}

int lfr_pid_get_d(void) {
    return D;
}

int lfr_pid_get_pidvalue(void) {
    return PIDvalue;
}

int lfr_pid_get_lsp(void) {
    return lsp;
}

int lfr_pid_get_rsp(void) {
    return rsp;
}

int lfr_pid_get_current_speed(void) {
    return currentSpeed;
}

int lfr_pid_get_last_turn_dir(void) {
    return lastTurnDir;
}
