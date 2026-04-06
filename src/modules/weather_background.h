#pragma once
#include "../trio_types.h"
#include <pebble.h>

/** Procedural sky / weather art behind the CGM graph (when weather is enabled). */
void weather_background_draw(GContext *ctx, GRect bounds, const Complications *comp, const TrioConfig *config);
