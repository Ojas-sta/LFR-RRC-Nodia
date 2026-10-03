#include "lfr_motor.h"
#include "lfr_bsp.h"

void motor1run(int motorSpeed) {
    lfr_bsp_motor1_set(motorSpeed);
}

void motor2run(int motorSpeed) {
    lfr_bsp_motor2_set(motorSpeed);
}

void stopMotors(void) {
    lfr_bsp_motors_stop();
}

void motors(int lsp, int rsp) {
    motor1run(lsp);
    motor2run(rsp);
}
