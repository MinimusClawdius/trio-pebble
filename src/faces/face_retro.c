// Face: Retro digital — Casio / classic LCD vibe inspired by fairly classic digital faces
// (e.g. 91 Dub style: https://github.com/orviwan/91-Dub-v2.0 — layout only, no shared assets).

#include "face_retro.h"
#include "../modules/complications.h"
#include "../modules/glucose_format.h"
#include "../modules/graph.h"
#include "../modules/platform_compat.h"
#include "../modules/time_display.h"
#include "../modules/trend_glyphs.h"

static TextLayer *s_glucose, *s_clock, *s_date;
static Layer *s_lcd_frame_layer, *s_graph_layer, *s_comp_layer, *s_trend_layer;
static char s_glucose_buf[16], s_clock_buf[16], s_date_buf[14];

static void graph_proc(Layer *layer, GContext *ctx) {
    graph_draw(layer, ctx, config_get());
}

static void comp_proc(Layer *layer, GContext *ctx) {
    complications_draw_bar(ctx, layer_get_bounds(layer), app_state_get(), config_get());
}

static void lcd_frame_proc(Layer *layer, GContext *ctx) {
    GRect b = layer_get_bounds(layer);
    bool light = config_get()->color_scheme == COLOR_SCHEME_LIGHT;
    GColor ink = light ? GColorBlack : GColorWhite;
    graphics_context_set_stroke_color(ctx, ink);
#ifdef PBL_COLOR
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_rect(ctx, GRect(0, 0, b.size.w, b.size.h));
    graphics_context_set_stroke_width(ctx, 1);
#else
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_rect(ctx, GRect(0, 0, b.size.w, b.size.h));
#endif
    graphics_draw_rect(ctx, GRect(2, 2, b.size.w - 4, b.size.h - 4));
    graphics_draw_rect(ctx, GRect(5, 5, b.size.w - 10, b.size.h - 10));
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

void face_retro_load(Window *window, Layer *root, GRect bounds) {
    (void)window;

    int w = bounds.size.w;
    int h = bounds.size.h;
    bool light = config_get()->color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;
    GColor fg2 = trio_secondary_fg(config_get());

    int half = w / 2;
    s_date = make_text(root, GRect(8, 4, half - 12, 16), FONT_KEY_GOTHIC_14, GTextAlignmentLeft, fg2);
    s_clock = make_text(root, GRect(half - 4, 0, half + 4, 32), FONT_KEY_GOTHIC_28_BOLD, GTextAlignmentRight, fg);

    int lcd_y = 24;
    int lcd_h = 44;
    s_lcd_frame_layer = layer_create(GRect(4, lcd_y, w - 8, lcd_h));
    layer_set_update_proc(s_lcd_frame_layer, lcd_frame_proc);
    layer_add_child(root, s_lcd_frame_layer);

    int inner_pad = 8;
    s_glucose = make_text(root, GRect(inner_pad, lcd_y + 4, w - 72 - inner_pad, 38), FONT_KEY_BITHAM_34_MEDIUM_NUMBERS,
                          GTextAlignmentLeft, fg);
    text_layer_set_text(s_glucose, "--");

    s_trend_layer = layer_create(GRect(w - 66, lcd_y + 6, 62, 34));
    layer_set_clips(s_trend_layer, true);
    layer_set_update_proc(s_trend_layer, trio_trend_layer_update_proc);
    layer_add_child(root, s_trend_layer);

    int graph_top = lcd_y + lcd_h + 4;
    int graph_h = h - graph_top - COMPLICATIONS_BAR_HEIGHT;
    if (graph_h < 28) {
        graph_h = 28;
    }
    s_graph_layer = layer_create(trio_graph_layer_bounds(bounds, graph_top, graph_h));
    layer_set_update_proc(s_graph_layer, graph_proc);
    layer_add_child(root, s_graph_layer);

    s_comp_layer = layer_create(
        GRect(TRIO_GRAPH_SIDE_INSET, h - COMPLICATIONS_BAR_HEIGHT, w - 2 * TRIO_GRAPH_SIDE_INSET, COMPLICATIONS_BAR_HEIGHT));
    layer_set_update_proc(s_comp_layer, comp_proc);
    layer_add_child(root, s_comp_layer);
}

void face_retro_unload(void) {
    text_layer_destroy(s_glucose);
    text_layer_destroy(s_clock);
    text_layer_destroy(s_date);
    layer_destroy(s_trend_layer);
    layer_destroy(s_lcd_frame_layer);
    layer_destroy(s_graph_layer);
    layer_destroy(s_comp_layer);
}

void face_retro_update(AppState *state) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    bool light = state->config.color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;
    GColor trend_ink = fg;

    trio_format_clock(s_clock_buf, sizeof(s_clock_buf), now, state->config.clock_24h);
    strftime(s_date_buf, sizeof(s_date_buf), "%a %d %b", t);
    text_layer_set_text(s_clock, s_clock_buf);
    text_layer_set_text(s_date, s_date_buf);

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
    }
#endif

    trio_trend_layer_set(state->cgm.trend_str, trend_ink, state->config.color_scheme == COLOR_SCHEME_LIGHT);
    layer_mark_dirty(s_trend_layer);

    layer_mark_dirty(s_lcd_frame_layer);
    layer_mark_dirty(s_graph_layer);
    layer_mark_dirty(s_comp_layer);
}
