#pragma once
#include "../trio_types.h"
#include <pebble.h>

void face_retro_load(Window *window, Layer *root, GRect bounds);
void face_retro_unload(void);
void face_retro_update(AppState *state);
