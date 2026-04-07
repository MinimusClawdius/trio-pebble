#pragma once
#include <pebble.h>
#include "../trio_types.h"

/**
 * Remote bolus / carb entry → AppMessage KEY_CMD_TYPE + KEY_CMD_AMOUNT → pkjs sendCommand().
 * CMD_TYPE 1 = bolus (amount = tenths of a unit, JS sends units = amount/10).
 * CMD_TYPE ≠ 1 = carbs (amount = grams).
 * Requires data source Trio (0) or Apple Health via Trio (3); JS rejects others.
 * Trio still confirms on the phone — this only starts the request.
 *
 * Invoked from main.c: long-press Select and long-press Down (Down avoids Quick Launch on Select).
 */
void remote_cmds_set_watchface_window(Window *watchface);
void remote_cmds_try_open(AppState *state);
