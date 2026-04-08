// Face: Graph Focus
// Maximizes graph area. Glucose overlaid on graph, minimal text.
// Designed for users who prioritize the trend visualization.

#include "face_graph_focus.h"
#include "../modules/graph.h"
#include "../modules/glucose_format.h"
#include "../modules/complications.h"
#include "../modules/platform_compat.h"
#include "../modules/time_display.h"
#include "../modules/trend_glyphs.h"

static TextLayer *s_glucose, *s_delta, *s_time;
static TextLayer *s_iob_cob;
static Layer *s_graph_layer, *s_trend_layer;
static char s_time_buf[16], s_glucose_buf[16], s_iob_cob_buf[48];

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

void face_graph_focus_load(Window *window, Layer *root, GRect bounds) {
    (void)window;
    int w = bounds.size.w;
    int h = bounds.size.h;
    bool light = config_get()->color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;
    GColor fg2 = trio_secondary_fg(config_get());

    // Full-height graph behind everything (inset on round watches)
    s_graph_layer = layer_create(trio_graph_layer_bounds(bounds, 0, h));
    layer_set_update_proc(s_graph_layer, graph_proc);
    layer_add_child(root, s_graph_layer);

    /* Row 0: BG (left) | fat trend glyph | time (right) — no overlap */
    s_glucose = make_text(root, GRect(2, 0, 44, 36), FONT_KEY_BITHAM_34_MEDIUM_NUMBERS, GTextAlignmentLeft, fg);
    text_layer_set_text(s_glucose, "--");

    s_trend_layer = layer_create(GRect(44, 2, 32, 32));
    layer_set_update_proc(s_trend_layer, trio_trend_layer_update_proc);
    layer_add_child(root, s_trend_layer);

    s_time = make_text(root, GRect(74, 0, w - 76, 32), FONT_KEY_GOTHIC_28_BOLD, GTextAlignmentRight, fg);

    s_delta = make_text(root, GRect(4, 34, w - 8, 18), FONT_KEY_GOTHIC_14, GTextAlignmentLeft, fg2);

    // IOB + COB combined - bottom left
    s_iob_cob = make_text(root, GRect(4, h - 18, w - 4, 16), FONT_KEY_GOTHIC_14, GTextAlignmentLeft, fg2);
}

void face_graph_focus_unload(void) {
    text_layer_destroy(s_glucose);
    text_layer_destroy(s_delta);
    text_layer_destroy(s_time);
    text_layer_destroy(s_iob_cob);
    layer_destroy(s_trend_layer);
    layer_destroy(s_graph_layer);
}

void face_graph_focus_update(AppState *state) {
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

    snprintf(s_iob_cob_buf, sizeof(s_iob_cob_buf), "IOB:%s COB:%s", state->loop.iob, state->loop.cob);
    text_layer_set_text(s_iob_cob, s_iob_cob_buf);

    layer_mark_dirty(s_graph_layer);
}
