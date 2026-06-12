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
#define CLASSIC_TIME_PAD_LEFT 8
#define CLASSIC_TIME_PAD_DOWN 8

static TextLayer *s_time, *s_age, *s_glucose;
static Layer *s_classic_chrome_layer, *s_graph_layer, *s_comp_layer, *s_trend_layer;
static char s_time_buf[16], s_age_buf[20];

// Centralized dynamic header height (matches skill recommendation)
static int get_dynamic_header_h(void) {
    TrioConfig *cfg = config_get();
    int hs = cfg ? cfg->header_size : 2;
    // More aggressive heights on emery for larger fonts
    if (trio_large_rect(layer_get_bounds(s_classic_chrome_layer))) {
        return (hs == 0) ? 32 : (hs == 1) ? 38 : 46;
    }
    return (hs == 0) ? 28 : (hs == 1) ? 32 : 40;
}

static void classic_chrome_proc(Layer *layer, GContext *ctx) {
    GRect wb = layer_get_bounds(layer);
    TrioConfig *cfg = config_get();
    if (!trio_classic_chrome_active(cfg)) {
        return;
    }
    int w = wb.size.w;
    int h = wb.size.h;
    int header_h = get_dynamic_header_h();
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
    layer_add_child(root, text_layer_get_layer(tl));
    return tl;
}

void face_classic_load(Window *window, Layer *root, GRect bounds) {
    (void)window;
    int w = bounds.size.w;
    int h = bounds.size.h;

    bool light = true;
    GColor fg = light ? GColorBlack : GColorWhite;
    GColor fg2 = trio_secondary_fg(config_get());
    bool chrome = trio_classic_chrome_active(config_get());
    GColor hdr_time = chrome ? (light ? GColorWhite : GColorBlack) : fg;
    GColor hdr_age = chrome ? (light ? GColorWhite : GColorDarkGray) : fg2;

    s_classic_chrome_layer = layer_create(bounds);
    layer_set_update_proc(s_classic_chrome_layer, classic_chrome_proc);
    layer_add_child(root, s_classic_chrome_layer);

    // Dynamic header + aggressive font sizing for emery
    int hs = config_get()->header_size;
    int dynamic_header_h = get_dynamic_header_h();
    APP_LOG(APP_LOG_LEVEL_INFO, "[CLASSIC] header_size=%d dynamic_header_h=%d emery=%d", hs, dynamic_header_h, trio_large_rect(bounds));
    int dynamic_header_h = get_dynamic_header_h();

    const char *time_font;
    const char *age_font;
    if (hs == 0) {
        time_font = FONT_KEY_GOTHIC_24_BOLD;
        age_font  = FONT_KEY_GOTHIC_14_BOLD;
    } else if (hs == 1) {
        time_font = FONT_KEY_GOTHIC_28_BOLD;
        age_font  = FONT_KEY_GOTHIC_18_BOLD;
    } else {
        time_font = FONT_KEY_GOTHIC_28_BOLD;
        age_font  = FONT_KEY_GOTHIC_18_BOLD;
    }

    int mid_y = dynamic_header_h / 2;
    int time_h = (hs == 0) ? 24 : 28;
    int age_h  = (hs == 0) ? 16 : 20;
    s_time = make_text(root, GRect(CLASSIC_TIME_PAD_LEFT, mid_y - time_h/2, w / 2 - CLASSIC_TIME_PAD_LEFT - 2, time_h),
                       time_font, GTextAlignmentLeft, hdr_time);
    s_age = make_text(root, GRect(w / 2, mid_y - age_h/2, w / 2 - 2, age_h), age_font, GTextAlignmentRight, hdr_age);

    /* Wider glucose column */
    int gw = w * 68 / 100;
    if (gw > w - 44) {
        gw = w - 44;
    }

    const char *glucose_font = FONT_KEY_BITHAM_42_BOLD;
    GColor hero_glucose = chrome ? (light ? GColorBlack : GColorWhite) : fg;

    s_glucose = make_text(root, GRect(8, dynamic_header_h + 4, gw - 16, 54 + 22),
                          glucose_font, GTextAlignmentLeft, hero_glucose);
    text_layer_set_text(s_glucose, "--");

    s_trend_layer = layer_create(GRect(gw, dynamic_header_h, w - gw, 54));
    layer_set_clips(s_trend_layer, true);
    layer_set_update_proc(s_trend_layer, trio_trend_layer_update_proc);
    layer_add_child(root, s_trend_layer);

    int graph_top = dynamic_header_h + 54;
    int graph_h = h - graph_top - COMPLICATIONS_BAR_HEIGHT;
    if (graph_h < 24) graph_h = 24;

    s_graph_layer = layer_create(GRect(0, graph_top, w, graph_h));
    layer_set_update_proc(s_graph_layer, graph_proc);
    layer_add_child(root, s_graph_layer);
    tap_framework_set_graph_bounds(GRect(0, graph_top, w, graph_h));

    s_comp_layer = layer_create(GRect(0, h - COMPLICATIONS_BAR_HEIGHT, w, COMPLICATIONS_BAR_HEIGHT));
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
}

void face_classic_update(AppState *state) {
    if (s_time) {
        trio_format_clock(s_time_buf, sizeof(s_time_buf), time(NULL), false);
        text_layer_set_text(s_time, s_time_buf);
    }
    if (s_age) {
        snprintf(s_age_buf, sizeof(s_age_buf), "now");
        text_layer_set_text(s_age, s_age_buf);
    }
}
