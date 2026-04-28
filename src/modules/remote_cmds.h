#pragma once
#include <pebble.h>
#include "../trio_types.h"

/**
 * Remote bolus / carb entry → AppMessage KEY_CMD_TYPE + KEY_CMD_AMOUNT → pkjs sendCommand().
 * CMD_TYPE 1 = bolus (amount = tenths of a unit, JS sends units = amount/10).
 * CMD_TYPE ≠ 1 = carbs (amount = grams).
 * Requires data source Trio (0) or Apple Health via Trio (3); JS rejects others.
 * Trio applies bolus/carbs immediately (no phone confirmation).
 *
 * Invoked from main.c: long-press Select, Up, or Down (Down/Up avoid Quick Launch on middle button).
 */
void remote_cmds_set_watchface_window(Window *watchface);
void remote_cmds_try_open(AppState *state);

/** After carbs, phone JS may send KEY_SUGGESTED_BOLUS_TENTHS to open bolus picker pre-filled (Loop-style). */
void remote_cmds_open_bolus_picker_preset(int32_t tenths);

/** Set the command status to display (called from main.c when KEY_CMD_STATUS received). */
void remote_cmds_set_status(const char *status);

/** Get the current command status. */
const char *remote_cmds_get_status(void);

/** Mark a command as pending. */
void remote_cmds_command_pending(void);

/** Check if a command is waiting for status. */
bool remote_cmds_is_command_pending(void);
