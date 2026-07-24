#include "face_classic.h"
#include "../modules/graph.h"
#include "../modules/complications.h"
#include "../modules/glucose_format.h"
#include "../modules/platform_compat.h"
#include "../modules/time_display.h"
#include "../modules/trend_glyphs.h"
#include "../modules/tap_framework.h"
#include <stdio.h>

#define CLASSIC_CARD_INSET 4
#define CLASSIC_CARD_RADIUS 4
#define CLASSIC_TIME_PAD_LEFT 6

static TextLayer *s_time, *s_age, *s_glucose;
static Layer *s_classic_chrome_layer, *s_graph_layer, *s_comp_layer, *s_trend_layer;
static char s_time_buf[16], s_glucose_buf[16], s_age_buf[20];
static GRect s_bounds;

/** Header strip height — scaled up for readable clocks on Emery. */
static int get_dynamic_header_h(GRect bounds) {
    TrioConfig *cfg = config_get();
    int hs = cfg ? cfg->header_size : 2;
    if (bounds.size.w >= 180) {
        /* was 32/38/46 — too short for large type */
        return (hs == 0) ? 40 : (hs == 1) ? 48 : 56;
    }
    return (hs == 0) ? 32 : (hs == 1) ? 36 : 42;
}

static int get_hero_h(GRect bounds) {
    TrioConfig *cfg = config_get();
    int hs = cfg ? cfg->header_size : 2;
    if (bounds.size.w >= 180) {
        return (hs == 0) ? 64 : (hs == 1) ? 72 : 80;
    }
    return (hs == 0) ? 50 : (hs == 1) ? 54 : 58;
}

static void classic_chrome_proc(Layer *layer, GContext *ctx) {
    GRect wb = layer_get_bounds(layer);
    TrioConfig *cfg = config_get();
    if (!trio_classic_chrome_active(cfg)) {
        return;
    }
    int w = wb.size.w;
    int h = wb.size.h;
    int header_h = get_dynamic_header_h(wb);
    int cy = header_h;
    int ch = h - header_h - COMPLICATIONS_BAR_HEIGHT;
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
    text_layer_set_overflow_mode(tl, GTextOverflowModeTrailingEllipsis);
    layer_add_child(root, text_layer_get_layer(tl));
    return tl;
}

void face_classic_load(Window *window, Layer *root, GRect bounds) {
    (void)window;
    s_bounds = bounds;
    int w = bounds.size.w;
    int h = bounds.size.h;

    bool light = config_get()->color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;
    GColor fg2 = trio_secondary_fg(config_get());
    bool chrome = trio_classic_chrome_active(config_get());
    /* Header strip: light scheme + chrome → white type on black bar; else scheme fg */
    GColor hdr_time;
    GColor hdr_age;
    if (chrome && light) {
        hdr_time = GColorWhite;
        hdr_age = GColorWhite;
    } else if (chrome && !light) {
        hdr_time = GColorBlack;
        hdr_age = GColorDarkGray;
    } else {
        hdr_time = fg;
        hdr_age = fg2;
    }

    s_classic_chrome_layer = layer_create(bounds);
    layer_set_update_proc(s_classic_chrome_layer, classic_chrome_proc);
    layer_add_child(root, s_classic_chrome_layer);

    int hs = config_get()->header_size;
    int dynamic_header_h = get_dynamic_header_h(bounds);
    int hero_h = get_hero_h(bounds);
    int trend_sz = trio_trend_size(bounds);

    /* Largest readable system fonts for header */
    const char *time_font = FONT_KEY_GOTHIC_28_BOLD;
    const char *age_font = (hs == 0) ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_24_BOLD;
    int time_h = 30;
    int age_h = (hs == 0) ? 20 : 26;

    int mid_y = dynamic_header_h / 2;
    s_time = make_text(root,
                       GRect(CLASSIC_TIME_PAD_LEFT, mid_y - time_h / 2,
                             w / 2 - CLASSIC_TIME_PAD_LEFT - 2, time_h),
                       time_font, GTextAlignmentLeft, hdr_time);
    s_age = make_text(root, GRect(w / 2, mid_y - age_h / 2, w / 2 - 4, age_h), age_font,
                      GTextAlignmentRight, hdr_age);

    /* Glucose takes ~62% width; trend glyph gets the rest (must stay large). */
    int trend_w = trend_sz + 8;
    if (trend_w < 56) trend_w = 56;
    if (trend_w > w / 3) trend_w = w / 3;
    int glucose_w = w - trend_w - CLASSIC_CARD_INSET * 2 - 8;
    int glucose_x = CLASSIC_CARD_INSET + 6;
    int hero_y = dynamic_header_h + 2;

    const char *glucose_font = trio_glucose_font(TRIO_DISPLAY_COLOR);
    GColor hero_glucose = chrome ? (light ? GColorBlack : GColorWhite) : fg;

    s_glucose = make_text(root, GRect(glucose_x, hero_y, glucose_w, hero_h - 4), glucose_font,
                          GTextAlignmentLeft, hero_glucose);
    text_layer_set_text(s_glucose, "--");

    int trend_x = w - CLASSIC_CARD_INSET - trend_w;
    int trend_y = hero_y + (hero_h - trend_sz) / 2;
    if (trend_y < hero_y) trend_y = hero_y;
    s_trend_layer = layer_create(GRect(trend_x, trend_y, trend_w, trend_sz));
    layer_set_clips(s_trend_layer, true);
    layer_set_update_proc(s_trend_layer, trio_trend_layer_update_proc);
    layer_add_child(root, s_trend_layer);

    int graph_top = dynamic_header_h + hero_h;
    int comp_y = h - COMPLICATIONS_BAR_HEIGHT;
    int graph_h = comp_y - graph_top;
    if (graph_h < 28) graph_h = 28;

    s_graph_layer = layer_create(GRect(0, graph_top, w, graph_h));
    layer_set_update_proc(s_graph_layer, graph_proc);
    layer_add_child(root, s_graph_layer);
    tap_framework_set_graph_bounds(GRect(0, graph_top, w, graph_h));

    s_comp_layer = layer_create(GRect(0, comp_y, w, COMPLICATIONS_BAR_HEIGHT));
    layer_set_update_proc(s_comp_layer, comp_proc);
    layer_add_child(root, s_comp_layer);
}

void face_classic_unload(void) {
    text_layer_destroy(s_time);
    text_layer_destroy(s_age);
    text_layer_destroy(s_glucose);
    layer_destroy(s_classic_chrome_layer);
    layer_destroy(s_graph_layer);
    layer_destroy(s_comp_layer);
    layer_destroy(s_trend_layer);
    s_time = s_age = s_glucose = NULL;
    s_classic_chrome_layer = s_graph_layer = s_comp_layer = s_trend_layer = NULL;
}

void face_classic_update(AppState *state) {
    if (!state) return;

    time_t now = time(NULL);
    bool light_scheme = state->config.color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light_scheme ? GColorBlack : GColorWhite;
    GColor trend_ink = fg;

    if (s_time) {
        trio_format_clock(s_time_buf, sizeof(s_time_buf), now, state->config.clock_24h);
        text_layer_set_text(s_time, s_time_buf);
    }

    if (s_age) {
        format_reading_age_upper(s_age_buf, sizeof(s_age_buf), state->cgm.last_reading_time, now);
        text_layer_set_text(s_age, s_age_buf);
    }

    if (s_glucose) {
        format_glucose_display(s_glucose_buf, sizeof(s_glucose_buf), state->cgm.glucose,
                               state->config.is_mmol);
        text_layer_set_text(s_glucose, s_glucose_buf);
#ifdef PBL_COLOR
        if (state->cgm.glucose > 0 && !trio_classic_chrome_active(&state->config)) {
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
        } else if (state->cgm.glucose > 0 && trio_classic_chrome_active(&state->config)) {
            /* On chrome card keep high-contrast black/white number; color the arrow. */
            bool light_pills = trio_classic_light_pills(&state->config);
            text_layer_set_text_color(s_glucose, light_pills ? GColorBlack : GColorWhite);
            TrioConfig *cfg = &state->config;
            if (state->cgm.glucose <= cfg->low_threshold)
                trend_ink = GColorRed;
            else if (state->cgm.glucose >= cfg->high_threshold)
                trend_ink = GColorOrange;
            else
                trend_ink = GColorGreen;
        }
#else
        (void)fg;
#endif
    }

    /* Critical: without this the trend layer never receives arrow data. */
    trio_trend_layer_set(state->cgm.trend_str, trend_ink,
                         trio_trend_light_background_assets(&state->config));
    if (s_trend_layer) {
        layer_mark_dirty(s_trend_layer);
    }
    if (s_graph_layer) {
        layer_mark_dirty(s_graph_layer);
    }
    if (s_comp_layer) {
        layer_mark_dirty(s_comp_layer);
    }
    if (s_classic_chrome_layer) {
        layer_mark_dirty(s_classic_chrome_layer);
    }
}
