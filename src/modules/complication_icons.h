#pragma once
#include "../trio_types.h"
#include <pebble.h>

/**
 * Rounded battery bar. If align_mid_y >= 0, vertical center is that Y (e.g. cell.origin.y + cell.size.h/2);
 * otherwise centered in icon_area.
 */
void trio_draw_footer_battery_bar(GContext *ctx, GRect icon_area, int align_mid_y, int pct, bool charging,
                                  GColor border_ink, const TrioConfig *config);

/** Small weather pictogram from `comp.weather_icon` tags (same as PKJS / weather_background). */
void trio_draw_footer_weather_icon(GContext *ctx, GRect icon_area, const char *weather_icon,
                                   const TrioConfig *config);

/** Simple insulin / drop glyph for IOB column. */
void trio_draw_footer_iob_icon(GContext *ctx, GRect icon_area, GColor ink);
