// Face: Classic — LOOP-style layout (Diorite / Emery targets)
// Header: time left, reading age right ("N MIN AGO").
// Hero: large glucose + trend arrow right.
// Graph: history window from settings (3–24h on phone); dotted thresholds, color bands on PBL_COLOR.
// Footer: complications bar (default: battery, weather, IOB).

#include "face_classic.h"
#include "../modules/graph.h"
#include "../modules/complications.h"
#include "../modules/glucose_format.h"
#include "../modules/platform_compat.h"
#include "../modules/time_display.h"
#include "../modules/trend_glyphs.h"

#define LOOP_HEADER_H 22
#define LOOP_HERO_H 54
#define LOOP_GRAPH_TOP (LOOP_HEADER_H + LOOP_HERO_H)

static TextLayer *s_time, *s_age, *s_glucose;
static Layer *s_graph_layer, *s_comp_layer, *s_trend_layer;
static char s_time_buf[16], s_glucose_buf[16], s_age_buf[20];

static void graph_proc(Layer *layer, GContext *ctx) {
    graph_draw(layer, ctx, config_get());
}

static void comp_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    complications_draw_bar(ctx, bounds, app_state_get(), config_get());
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

void face_classic_load(Window *window, Layer *root, GRect bounds) {
    (void)window;
    int w = bounds.size.w;
    int h = bounds.size.h;
    bool light = config_get()->color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;
    GColor fg2 = trio_secondary_fg(config_get());

    s_time = make_text(root, GRect(0, 0, w / 2 - 2, LOOP_HEADER_H), FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft, fg);
    s_age = make_text(root, GRect(w / 2, 0, w / 2 - 2, LOOP_HEADER_H), FONT_KEY_GOTHIC_14_BOLD,
                      GTextAlignmentRight, fg2);

    int gw = w * 54 / 100;
#ifdef PBL_COLOR
    const char *glucose_font = FONT_KEY_ROBOTO_BOLD_SUBSET_49;
#else
    const char *glucose_font = FONT_KEY_BITHAM_42_BOLD;
#endif
    s_glucose = make_text(root, GRect(0, LOOP_HEADER_H, gw, LOOP_HERO_H), glucose_font, GTextAlignmentLeft, fg);
    text_layer_set_text(s_glucose, "--");

    s_trend_layer = layer_create(GRect(gw, LOOP_HEADER_H, w - gw, LOOP_HERO_H));
    layer_set_clips(s_trend_layer, true);
    layer_set_update_proc(s_trend_layer, trio_trend_layer_update_proc);
    layer_add_child(root, s_trend_layer);

    int graph_h = h - LOOP_GRAPH_TOP - COMPLICATIONS_BAR_HEIGHT;
    if (graph_h < 24) {
        graph_h = 24;
    }
    s_graph_layer = layer_create(GRect(0, LOOP_GRAPH_TOP, w, graph_h));
    layer_set_update_proc(s_graph_layer, graph_proc);
    layer_add_child(root, s_graph_layer);

    s_comp_layer = layer_create(GRect(0, h - COMPLICATIONS_BAR_HEIGHT, w, COMPLICATIONS_BAR_HEIGHT));
    layer_set_update_proc(s_comp_layer, comp_proc);
    layer_add_child(root, s_comp_layer);
}

void face_classic_unload(void) {
    text_layer_destroy(s_time);
    text_layer_destroy(s_age);
    text_layer_destroy(s_glucose);
    layer_destroy(s_trend_layer);
    layer_destroy(s_graph_layer);
    layer_destroy(s_comp_layer);
}

void face_classic_update(AppState *state) {
    time_t now = time(NULL);
    bool light = state->config.color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;
    GColor trend_ink = fg;

    trio_format_clock(s_time_buf, sizeof(s_time_buf), now, state->config.clock_24h);
    text_layer_set_text(s_time, s_time_buf);

    format_reading_age_upper(s_age_buf, sizeof(s_age_buf), state->cgm.last_reading_time, now);
    text_layer_set_text(s_age, s_age_buf);

    format_glucose_display(s_glucose_buf, sizeof(s_glucose_buf), state->cgm.glucose, state->config.is_mmol);
    text_layer_set_text(s_glucose, s_glucose_buf);

#ifdef PBL_COLOR
    if (state->cgm.glucose > 0) {
        TrioConfig *cfg = &state->config;
        GColor gc;
        if (state->cgm.glucose <= cfg->urgent_low) {
            gc = GColorRed;
        } else if (state->cgm.glucose <= cfg->low_threshold) {
            gc = GColorRed;
        } else if (state->cgm.glucose >= cfg->high_threshold) {
            gc = GColorOrange;
        } else {
            gc = GColorGreen;
        }
        text_layer_set_text_color(s_glucose, gc);
        trend_ink = gc;
    } else {
        text_layer_set_text_color(s_glucose, fg);
    }
#endif

    trio_trend_layer_set(state->cgm.trend_str, trend_ink, state->config.color_scheme == COLOR_SCHEME_LIGHT);
    layer_mark_dirty(s_trend_layer);

    layer_mark_dirty(s_graph_layer);
    layer_mark_dirty(s_comp_layer);
}
