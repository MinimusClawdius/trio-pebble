// Face: Compact
// Two-row data + graph. Similar to T1000 design.
// Row 1: large glucose + trend arrow
// Row 2: delta, time since reading
// Graph fills remaining space with clear threshold lines.

#include "face_compact.h"
#include "../modules/graph.h"
#include "../modules/glucose_format.h"
#include "../modules/platform_compat.h"
#include "../modules/time_display.h"
#include "../modules/trend_glyphs.h"

static TextLayer *s_glucose, *s_delta, *s_age, *s_time;
static Layer *s_graph_layer, *s_trend_layer;
static char s_time_buf[16], s_glucose_buf[16], s_age_buf[16];

static void graph_proc(Layer *layer, GContext *ctx) {
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

void face_compact_load(Window *window, Layer *root, GRect bounds) {
    (void)window;
    int w = bounds.size.w;
    int h = bounds.size.h;
    bool light = config_get()->color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;
    GColor fg2 = trio_secondary_fg(config_get());

    s_glucose = make_text(root, GRect(4, -4, 48, 40), FONT_KEY_BITHAM_34_MEDIUM_NUMBERS, GTextAlignmentLeft, fg);
    text_layer_set_text(s_glucose, "--");

    s_trend_layer = layer_create(GRect(50, 0, 32, 34));
    layer_set_update_proc(s_trend_layer, trio_trend_layer_update_proc);
    layer_add_child(root, s_trend_layer);

    int col1 = w * 36 / 100;
    int col2 = w * 34 / 100;
    s_delta = make_text(root, GRect(0, 40, col1, 16), FONT_KEY_GOTHIC_14, GTextAlignmentCenter, fg2);
    s_age = make_text(root, GRect(col1, 40, col2, 16), FONT_KEY_GOTHIC_14, GTextAlignmentCenter, fg2);
    s_time = make_text(root, GRect(col1 + col2, 36, w - col1 - col2, 26), FONT_KEY_GOTHIC_28_BOLD, GTextAlignmentCenter, fg2);

    // Graph - fill the rest
    int graph_top = 60;
    s_graph_layer = layer_create(trio_graph_layer_bounds(bounds, graph_top, h - graph_top - 2));
    layer_set_update_proc(s_graph_layer, graph_proc);
    layer_add_child(root, s_graph_layer);
}

void face_compact_unload(void) {
    text_layer_destroy(s_glucose);
    text_layer_destroy(s_delta);
    text_layer_destroy(s_age);
    text_layer_destroy(s_time);
    layer_destroy(s_trend_layer);
    layer_destroy(s_graph_layer);
}

void face_compact_update(AppState *state) {
    time_t now = time(NULL);
    bool light = state->config.color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;
    GColor trend_ink = fg;

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
#endif

    trio_trend_layer_set(state->cgm.trend_str, trend_ink, state->config.color_scheme == COLOR_SCHEME_LIGHT);
    layer_mark_dirty(s_trend_layer);

    text_layer_set_text(s_delta, state->cgm.delta_str);

    // Reading age
    if (state->cgm.last_reading_time > 0) {
        int age_min = (int)(now - state->cgm.last_reading_time) / 60;
        snprintf(s_age_buf, sizeof(s_age_buf), "%dm ago", age_min);
    } else {
        snprintf(s_age_buf, sizeof(s_age_buf), "--");
    }
    text_layer_set_text(s_age, s_age_buf);

    layer_mark_dirty(s_graph_layer);
}
