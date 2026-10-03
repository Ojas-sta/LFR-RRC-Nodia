#pragma once

#ifndef LFR_PID_H
#define LFR_PID_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// PID lifecycle & execution
void lfr_pid_reset(int startSpeed);
void linefollow(const LFRConfig *cfg);

// Turn and speed dynamics
void lfr_handle_sharp_turns_and_speed(const LFRConfig *cfg);
void lfr_handle_lost_line(const LFRConfig *cfg);

// Diagnostic accessors
double lfr_pid_get_error(void);
int    lfr_pid_get_p(void);
int    lfr_pid_get_i(void);
int    lfr_pid_get_d(void);
int    lfr_pid_get_pidvalue(void);
int    lfr_pid_get_lsp(void);
int    lfr_pid_get_rsp(void);
int    lfr_pid_get_current_speed(void);
int    lfr_pid_get_last_turn_dir(void);

#ifdef __cplusplus
}
#endif

#endif // LFR_PID_H
