// Face: Minimal
// Clean everyday layout: large clock, large glucose, trend + delta, sparkline.
// Stacked (no overlap) with Emery-sized type.

#include "face_minimal.h"
#include "../modules/graph.h"
#include "../modules/glucose_format.h"
#include "../modules/platform_compat.h"
#include "../modules/time_display.h"
#include "../modules/trend_glyphs.h"
#include <stdio.h>

static TextLayer *s_time, *s_glucose, *s_delta;
static Layer *s_sparkline_layer, *s_trend_layer;
static char s_time_buf[16], s_glucose_buf[16];

static void sparkline_proc(Layer *layer, GContext *ctx) {
    graph_draw(layer, ctx, config_get());
}

static TextLayer *make_text(Layer *root, GRect frame, const char *font_key, GTextAlignment align,
                             GColor fg) {
    TextLayer *tl = text_layer_create(frame);
    text_layer_set_background_color(tl, GColorClear);
    text_layer_set_text_color(tl, fg);
    text_layer_set_font(tl, fonts_get_system_font(font_key));
    text_layer_set_text_alignment(tl, align);
    text_layer_set_overflow_mode(tl, GTextOverflowModeTrailingEllipsis);
    layer_add_child(root, text_layer_get_layer(tl));
    return tl;
}

void face_minimal_load(Window *window, Layer *root, GRect bounds) {
    (void)window;
    int w = bounds.size.w;
    int h = bounds.size.h;
    bool light = config_get()->color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;
    GColor fg2 = light ? GColorDarkGray : GColorLightGray;
    bool large = trio_large_rect(bounds);

    /* Vertical stack (no overlapping centers):
     *  [clock]
     *  [glucose hero]
     *  [trend + delta]
     *  [sparkline]
     */
    int pad = large ? 6 : 4;
    int clock_h = large ? 42 : 32;
    int glucose_h = large ? 42 : 32;
    int trend_sz = trio_trend_size(bounds);
    if (trend_sz > 32) trend_sz = 32;
    int mid_row_h = trend_sz + 4;
    int spark_h = large ? 36 : 28;
    int spark_y = h - spark_h - pad;

    int y = pad;
    s_time = make_text(root, GRect(pad, y, w - 2 * pad, clock_h), FONT_KEY_GOTHIC_32_BOLD,
                       GTextAlignmentCenter, fg);
    y += clock_h + (large ? 4 : 2);

    s_glucose = make_text(root, GRect(pad, y, w - 2 * pad, glucose_h),
                          trio_glucose_font(TRIO_DISPLAY_COLOR), GTextAlignmentCenter, fg);
    text_layer_set_text(s_glucose, "--");
    y += glucose_h + (large ? 2 : 2);

    /* Trend centered; delta to its right (or under if narrow). */
    int trend_x = (w - trend_sz) / 2 - (large ? 18 : 10);
    if (trend_x < pad) trend_x = pad;
    s_trend_layer = layer_create(GRect(trend_x, y, trend_sz, trend_sz));
    layer_set_clips(s_trend_layer, true);
    layer_set_update_proc(s_trend_layer, trio_trend_layer_update_proc);
    layer_add_child(root, s_trend_layer);

    int delta_x = trend_x + trend_sz + 6;
    int delta_w = w - delta_x - pad;
    if (delta_w < 40) {
        /* fallback: delta under trend */
        delta_x = pad;
        delta_w = w - 2 * pad;
        s_delta = make_text(root, GRect(delta_x, y + trend_sz + 2, delta_w, 24),
                            FONT_KEY_GOTHIC_24_BOLD, GTextAlignmentCenter, fg2);
        y += mid_row_h + 26;
    } else {
        s_delta = make_text(root, GRect(delta_x, y + (trend_sz - 28) / 2, delta_w, 28),
                            FONT_KEY_GOTHIC_24_BOLD, GTextAlignmentLeft, fg2);
        y += mid_row_h + 4;
    }

    /* Keep sparkline from eating mid content */
    if (y > spark_y - 4) {
        spark_y = y + 2;
        spark_h = h - spark_y - pad;
        if (spark_h < 20) spark_h = 20;
    }

    s_sparkline_layer = layer_create(trio_graph_layer_bounds(bounds, spark_y, spark_h));
    layer_set_update_proc(s_sparkline_layer, sparkline_proc);
    layer_add_child(root, s_sparkline_layer);
}

void face_minimal_unload(void) {
    text_layer_destroy(s_time);
    text_layer_destroy(s_glucose);
    text_layer_destroy(s_delta);
    layer_destroy(s_trend_layer);
    layer_destroy(s_sparkline_layer);
    s_time = s_glucose = s_delta = NULL;
    s_trend_layer = s_sparkline_layer = NULL;
}

void face_minimal_update(AppState *state) {
    if (!state) return;

    time_t now = time(NULL);
    bool light = state->config.color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;
    GColor fg2 = light ? GColorDarkGray : GColorLightGray;
    GColor trend_ink = fg;

    trio_format_clock(s_time_buf, sizeof(s_time_buf), now, state->config.clock_24h);
    text_layer_set_text(s_time, s_time_buf);
    text_layer_set_text_color(s_time, fg);

    format_glucose_display(s_glucose_buf, sizeof(s_glucose_buf), state->cgm.glucose,
                           state->config.is_mmol);
    text_layer_set_text(s_glucose, s_glucose_buf);

#ifdef PBL_COLOR
    if (state->cgm.glucose > 0) {
        TrioConfig *cfg = &state->config;
        GColor gc;
        if (state->cgm.glucose <= cfg->low_threshold)
            gc = GColorRed;
        else if (state->cgm.glucose >= cfg->high_threshold)
            gc = GColorOrange;
        else
            gc = GColorGreen;
        text_layer_set_text_color(s_glucose, gc);
        trend_ink = gc;
    } else {
        text_layer_set_text_color(s_glucose, fg);
        trend_ink = fg;
    }
#else
    text_layer_set_text_color(s_glucose, fg);
    trend_ink = fg;
#endif

    trio_trend_layer_set(state->cgm.trend_str, trend_ink,
                         trio_trend_light_background_assets(&state->config));
    if (s_trend_layer) {
        layer_mark_dirty(s_trend_layer);
    }

    if (s_delta) {
        text_layer_set_text(s_delta, state->cgm.delta_str[0] ? state->cgm.delta_str : "--");
        text_layer_set_text_color(s_delta, fg2);
    }

    if (s_sparkline_layer) {
        layer_mark_dirty(s_sparkline_layer);
    }
}
