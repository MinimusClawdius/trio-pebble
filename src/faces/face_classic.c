// Face: Classic — LOOP-style layout (Diorite / Emery targets)
// Light: black header/footer (square to bezel); white rounded center card (hero + trend + graph).
// Dark: inverse — white bars, black rounded center card.
// High contrast: no chrome layer (flat legacy layout).

#include "face_classic.h"
#include "../modules/graph.h"
#include "../modules/complications.h"
#include "../modules/glucose_format.h"
#include "../modules/platform_compat.h"
#include "../modules/time_display.h"
#include "../modules/trend_glyphs.h"

#define LOOP_HEADER_H 24
#define LOOP_HERO_H 54
#define LOOP_GRAPH_TOP (LOOP_HEADER_H + LOOP_HERO_H)
#define CLASSIC_CARD_INSET 2
#define CLASSIC_CARD_RADIUS 5
#define CLASSIC_TIME_PAD_LEFT 8

static TextLayer *s_time, *s_age, *s_glucose;
static Layer *s_classic_chrome_layer, *s_graph_layer, *s_comp_layer, *s_trend_layer;
static char s_time_buf[16], s_glucose_buf[16], s_age_buf[20];

static void classic_chrome_proc(Layer *layer, GContext *ctx) {
    GRect wb = layer_get_bounds(layer);
    TrioConfig *cfg = config_get();
    if (!trio_classic_chrome_active(cfg)) {
        return;
    }
    int w = wb.size.w;
    int h = wb.size.h;
    int cy = LOOP_HEADER_H;
    int ch = h - LOOP_HEADER_H - COMPLICATIONS_BAR_HEIGHT;
    GRect card = GRect(CLASSIC_CARD_INSET, cy, w - 2 * CLASSIC_CARD_INSET, ch);
    GCornerMask card_corners =
        (GCornerMask)(GCornerTopLeft | GCornerTopRight | GCornerBottomLeft | GCornerBottomRight);

    if (trio_classic_light_pills(cfg)) {
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_fill_rect(ctx, wb, 0, GCornerNone);
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_fill_rect(ctx, card, CLASSIC_CARD_RADIUS, card_corners);
        return;
    }
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, wb, 0, GCornerNone);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, card, CLASSIC_CARD_RADIUS, card_corners);
}

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
    bool chrome = trio_classic_chrome_active(config_get());
    GColor hdr_time = chrome ? (light ? GColorWhite : GColorBlack) : fg;
    GColor hdr_age = chrome ? (light ? GColorWhite : GColorDarkGray) : fg2;

    s_classic_chrome_layer = layer_create(bounds);
    layer_set_update_proc(s_classic_chrome_layer, classic_chrome_proc);
    layer_add_child(root, s_classic_chrome_layer);

    s_time = make_text(root, GRect(CLASSIC_TIME_PAD_LEFT, 0, w / 2 - CLASSIC_TIME_PAD_LEFT - 2, LOOP_HEADER_H),
                       FONT_KEY_GOTHIC_14_BOLD, GTextAlignmentLeft, hdr_time);
    s_age = make_text(root, GRect(w / 2, 0, w / 2 - 2, LOOP_HEADER_H), FONT_KEY_GOTHIC_14_BOLD, GTextAlignmentRight,
                      hdr_age);

    /* Wider glucose column so "10.2" mmol does not ellipsize */
    int gw = w * 62 / 100;
    if (gw > w - 44) {
        gw = w - 44;
    }
#ifdef PBL_COLOR
    const char *glucose_font = FONT_KEY_ROBOTO_BOLD_SUBSET_49;
#else
    const char *glucose_font = FONT_KEY_BITHAM_42_BOLD;
#endif
    GColor hero_glucose = fg;
    if (chrome) {
        hero_glucose = light ? GColorBlack : GColorWhite;
    }
    s_glucose = make_text(root, GRect(0, LOOP_HEADER_H, gw, LOOP_HERO_H), glucose_font, GTextAlignmentLeft,
                          hero_glucose);
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
    layer_destroy(s_classic_chrome_layer);
    layer_destroy(s_trend_layer);
    layer_destroy(s_graph_layer);
    layer_destroy(s_comp_layer);
}

void face_classic_update(AppState *state) {
    time_t now = time(NULL);
    bool light = state->config.color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;
    GColor trend_ink = fg;
    bool chrome = trio_classic_chrome_active(&state->config);

    trio_format_clock(s_time_buf, sizeof(s_time_buf), now, state->config.clock_24h);
    text_layer_set_text(s_time, s_time_buf);

    format_reading_age_upper(s_age_buf, sizeof(s_age_buf), state->cgm.last_reading_time, now);
    text_layer_set_text(s_age, s_age_buf);

    if (chrome) {
        if (light) {
            text_layer_set_text_color(s_time, GColorWhite);
            text_layer_set_text_color(s_age, GColorWhite);
        } else {
            text_layer_set_text_color(s_time, GColorBlack);
            text_layer_set_text_color(s_age, GColorDarkGray);
        }
    } else {
        text_layer_set_text_color(s_time, fg);
        text_layer_set_text_color(s_age, trio_secondary_fg(&state->config));
    }

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
        text_layer_set_text_color(s_glucose, chrome ? (light ? GColorBlack : GColorWhite) : fg);
    }
#else
    text_layer_set_text_color(s_glucose, chrome ? (light ? GColorBlack : GColorWhite) : fg);
#endif

    trio_trend_layer_set(state->cgm.trend_str, trend_ink, trio_trend_light_background_assets(&state->config));
    layer_mark_dirty(s_trend_layer);

    layer_mark_dirty(s_classic_chrome_layer);
    layer_mark_dirty(s_graph_layer);
    layer_mark_dirty(s_comp_layer);
}
