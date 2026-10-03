#pragma once

#ifndef LFR_MOTOR_H
#define LFR_MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

// Direct motor control primitives (preserves exact golden reference names and behavior)
void motor1run(int motorSpeed); // Motor 1 = A (Left wheel)
void motor2run(int motorSpeed); // Motor 2 = B (Right wheel)
void stopMotors(void);
void motors(int lsp, int rsp);

#ifdef __cplusplus
}
#endif

#endif // LFR_MOTOR_H
