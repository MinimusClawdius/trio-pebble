#pragma once
#include <pebble.h>

/**
 * Draws a bold trend glyph (CGM-style) into bounds. Parses UTF-8 arrows from
 * trio_normalize_trend_str / pkjs; falls back to graphics_draw_text for "--", "?", etc.
 */
void trio_trend_glyph_draw(GContext *ctx, GRect bounds, const char *utf8, GColor ink);

/** Layer update_proc: call trio_trend_layer_set() before layer_mark_dirty. */
void trio_trend_layer_set(const char *utf8, GColor ink);
void trio_trend_layer_update_proc(Layer *layer, GContext *ctx);
