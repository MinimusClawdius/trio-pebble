// Face: Classic
// Layout: Time top-right, large glucose center-top, trend+delta,
//         IOB/COB row, graph center, loop status, complications bar bottom.

#include "face_classic.h"
#include "../modules/graph.h"
#include "../modules/complications.h"
#include "../modules/glucose_format.h"
#include "../modules/platform_compat.h"

static TextLayer *s_time, *s_glucose, *s_trend, *s_delta;
static TextLayer *s_iob, *s_cob, *s_loop;
static Layer *s_graph_layer, *s_comp_layer;
static char s_time_buf[8], s_glucose_buf[16];

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

    s_time = make_text(root, GRect(w - 66, 0, 62, 28), FONT_KEY_BITHAM_30_BLACK, GTextAlignmentRight, fg2);

    {
        s_glucose = make_text(root, GRect(4, -2, 56, 44), FONT_KEY_BITHAM_34_MEDIUM_NUMBERS, GTextAlignmentLeft, fg);
        text_layer_set_text(s_glucose, "--");
        s_trend = make_text(root, GRect(58, 0, 42, 42), FONT_KEY_GOTHIC_28_BOLD, GTextAlignmentLeft, fg);
    }

    s_delta = make_text(root, GRect(0, 42, w / 2, 16), FONT_KEY_GOTHIC_14, GTextAlignmentCenter, fg2);

#ifdef PBL_COLOR
    s_iob = make_text(root, GRect(w / 2, 42, w / 4, 16), FONT_KEY_GOTHIC_14, GTextAlignmentCenter, GColorCyan);
    s_cob = make_text(root, GRect(3 * w / 4, 42, w / 4, 16), FONT_KEY_GOTHIC_14, GTextAlignmentCenter, GColorOrange);
#else
    s_iob = make_text(root, GRect(w / 2, 42, w / 4, 16), FONT_KEY_GOTHIC_14, GTextAlignmentCenter, fg);
    s_cob = make_text(root, GRect(3 * w / 4, 42, w / 4, 16), FONT_KEY_GOTHIC_14, GTextAlignmentCenter, fg);
#endif

    // Graph (shorter bottom margin for taller battery/weather bar)
    int graph_top = 60;
    int graph_h = h - graph_top - 16 - COMPLICATIONS_BAR_HEIGHT;
    s_graph_layer = layer_create(trio_graph_layer_bounds(bounds, graph_top, graph_h));
    layer_set_update_proc(s_graph_layer, graph_proc);
    layer_add_child(root, s_graph_layer);

    // Loop status
    s_loop = make_text(root, GRect(0, h - 16 - COMPLICATIONS_BAR_HEIGHT, w, 16), FONT_KEY_GOTHIC_14, GTextAlignmentCenter, fg2);

    // Complications bar (large battery & temperature)
    s_comp_layer = layer_create(
        GRect(TRIO_GRAPH_SIDE_INSET, h - COMPLICATIONS_BAR_HEIGHT, w - 2 * TRIO_GRAPH_SIDE_INSET, COMPLICATIONS_BAR_HEIGHT));
    layer_set_update_proc(s_comp_layer, comp_proc);
    layer_add_child(root, s_comp_layer);
}

void face_classic_unload(void) {
    text_layer_destroy(s_time);
    text_layer_destroy(s_glucose);
    text_layer_destroy(s_trend);
    text_layer_destroy(s_delta);
    text_layer_destroy(s_iob);
    text_layer_destroy(s_cob);
    text_layer_destroy(s_loop);
    layer_destroy(s_graph_layer);
    layer_destroy(s_comp_layer);
}

void face_classic_update(AppState *state) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(s_time_buf, sizeof(s_time_buf), "%H:%M", t);
    text_layer_set_text(s_time, s_time_buf);

    format_glucose_display(s_glucose_buf, sizeof(s_glucose_buf), state->cgm.glucose,
                           state->config.is_mmol);
    text_layer_set_text(s_glucose, s_glucose_buf);

    // Color glucose based on range
#ifdef PBL_COLOR
    if (state->cgm.glucose > 0) {
        TrioConfig *cfg = &state->config;
        GColor gc;
        if (state->cgm.glucose <= cfg->urgent_low) gc = GColorRed;
        else if (state->cgm.glucose <= cfg->low_threshold) gc = GColorRed;
        else if (state->cgm.glucose >= cfg->high_threshold) gc = GColorOrange;
        else gc = GColorGreen;
        text_layer_set_text_color(s_glucose, gc);
        text_layer_set_text_color(s_trend, gc);
    }
#endif

    text_layer_set_text(s_trend, state->cgm.trend_str);
    text_layer_set_text(s_delta, state->cgm.delta_str);
    text_layer_set_text(s_iob, state->loop.iob);
    text_layer_set_text(s_cob, state->loop.cob);
    text_layer_set_text(s_loop, state->loop.last_loop_time);

    layer_mark_dirty(s_graph_layer);
    layer_mark_dirty(s_comp_layer);
}
