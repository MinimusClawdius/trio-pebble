// Face: Minimal
// Clean, elegant display with just glucose, trend, time.
// Perfect for everyday wear when you want a watch that
// happens to show glucose. Subtle and non-medical looking.

#include "face_minimal.h"
#include "../modules/graph.h"
#include "../modules/glucose_format.h"
#include "../modules/platform_compat.h"
#include "../modules/time_display.h"
#include "../modules/trend_glyphs.h"

static TextLayer *s_time, *s_glucose, *s_delta;
static Layer *s_sparkline_layer, *s_trend_layer;
static char s_time_buf[16], s_glucose_buf[16];

static void sparkline_proc(Layer *layer, GContext *ctx) {
    graph_draw(layer, ctx, config_get());
}

static TextLayer *make_text(Layer *root, GRect frame, const char *font_key, GTextAlignment align, GColor fg) {
    TextLayer *tl = text_layer_create(frame);
    text_layer_set_background_color(tl, GColorClear);
    text_layer_set_text_color(tl, fg);
    text_layer_set_font(tl, fonts_get_system_font(font_key));
    text_layer_set_text_alignment(tl, align);
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

    // Glucose above — prominent
    s_glucose = make_text(root, GRect(0, h / 2 - 66, w, 28), FONT_KEY_GOTHIC_28_BOLD, GTextAlignmentCenter, fg);
    text_layer_set_text(s_glucose, "--");

    /* Large clock (SIMPLE CGM–style): full width for 12h + AM/PM */
    s_time = make_text(root, GRect(4, h / 2 - 34, w - 8, 36), FONT_KEY_GOTHIC_28_BOLD, GTextAlignmentCenter, fg);

    s_trend_layer = layer_create(GRect(w / 2 - 40, h / 2 + 8, 36, 34));
    layer_set_update_proc(s_trend_layer, trio_trend_layer_update_proc);
    layer_add_child(root, s_trend_layer);

    s_delta = make_text(root, GRect(w / 2 + 4, h / 2 + 12, w / 2 - 12, 22), FONT_KEY_GOTHIC_18, GTextAlignmentLeft, fg2);

    // Thin sparkline at bottom
    s_sparkline_layer = layer_create(trio_graph_layer_bounds(bounds, h - 30, 24));
    layer_set_update_proc(s_sparkline_layer, sparkline_proc);
    layer_add_child(root, s_sparkline_layer);
}

void face_minimal_unload(void) {
    text_layer_destroy(s_time);
    text_layer_destroy(s_glucose);
    text_layer_destroy(s_delta);
    layer_destroy(s_trend_layer);
    layer_destroy(s_sparkline_layer);
}

void face_minimal_update(AppState *state) {
    time_t now = time(NULL);
    bool light = state->config.color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;
    GColor fg2 = light ? GColorDarkGray : GColorLightGray;
    GColor trend_ink = fg2;

    trio_format_clock(s_time_buf, sizeof(s_time_buf), now, state->config.clock_24h);
    text_layer_set_text(s_time, s_time_buf);

    format_glucose_display(s_glucose_buf, sizeof(s_glucose_buf), state->cgm.glucose,
                           state->config.is_mmol);
    text_layer_set_text(s_glucose, s_glucose_buf);

#ifdef PBL_COLOR
    if (state->cgm.glucose > 0) {
        TrioConfig *cfg = &state->config;
        GColor gc;
        if (state->cgm.glucose <= cfg->low_threshold) gc = GColorRed;
        else if (state->cgm.glucose >= cfg->high_threshold) gc = GColorOrange;
        else gc = GColorGreen;
        text_layer_set_text_color(s_glucose, gc);
        trend_ink = gc;
    }
#else
    trend_ink = fg;
#endif

    trio_trend_layer_set(state->cgm.trend_str, trend_ink);
    layer_mark_dirty(s_trend_layer);

    text_layer_set_text(s_delta, state->cgm.delta_str);

    layer_mark_dirty(s_sparkline_layer);
}
