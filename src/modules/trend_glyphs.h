#pragma once
#include <pebble.h>

/**
 * Draws a bold trend glyph (CGM-style) into bounds. Parses UTF-8 arrows from
 * trio_normalize_trend_str / pkjs; falls back to graphics_draw_text for "--", "?", etc.
 */
void trio_trend_glyph_draw(GContext *ctx, GRect bounds, const char *utf8, GColor ink);

/**
 * Layer update_proc: call trio_trend_layer_set() before layer_mark_dirty.
 * light_color_scheme: true for COLOR_SCHEME_LIGHT → dark arrow PNGs (*_black) on white background.
 */
void trio_trend_layer_set(const char *utf8, GColor ink, bool light_color_scheme);
void trio_trend_layer_update_proc(Layer *layer, GContext *ctx);
void trio_trend_glyphs_deinit(void);
